/*
 * File:   bt_bm83_pbap.c
 * Author: Matt C <gumbysjunk@hotmail.com>
 * Description:
 *  Implementation of phone book access for the Microchip BM83 Bluetooth UART API
 *  Attempts to enable the existing phone functions including:
 *  - Directory (access one at a time for IKE/MID, or 8 at a time for BM)
 *  - Top-8 (favorites)
 *  - Last Numbers (call history)
 * 
 * Assumes that there is not enough memory available to download the entire
 * phone book on connection (TODO - test this assumption?)
 * 
 * Data storage uses a fixed allocation of PHONE_BOOK_MAX_ENTRIES (8)
 * each with up to PHONE_BOOK_MAX_NUMBERS (3) - to match the BMBT display.
 *  
 *  Not yet attempted:
 *  - SMS messages
 *  - Adding menus
 *  - Updating dial buffer with selected contact (bt->dialBuffer)?
 * 
 * Revision History:
 *      Feb 2024 (Matt C) - Initial build
 */

#include <string.h>
#include "../utils.h"
#include "./bt_bm83.h"
#include "./bt_bm83_pbap.h"
#include "../event.h"

// Simple state machine
enum Phonebook_Status {
    Disconnected,
    Connected,
    Waiting     
};
static enum Phonebook_Status _pb_status = Disconnected;
static struct {
    uint8_t isValid;
    uint8_t type;
    uint16_t offset;
} _pb_onConnect;

// Phonebook message buffer (to assemble multi-part messages)
static uint8_t  _pb_buffer[PHONE_BOOK_MAX_BUFFER];
static uint16_t _pb_buffer_offset = 0;

// TO DO: Strings that should be localised...
const char* DEFAULT_TEL_TYPE_NAME = "TEL";

static struct PHONEBOOK_ENTRY_TYPE _phonebook[PHONE_BOOK_MAX_ENTRIES];


//
// Event packet header structures
//

// common header shapes...
struct PBAPC_HEADER_ID {
    uint8_t device_id;    
};
struct PBAPC_HEADER_ID_STATUS {
    uint8_t device_id;
    uint8_t status;
};
// header for receiving phone book or vCard lists
struct PBAPC_HEADER_PHONEBOOK {
    uint8_t device_id;
    uint8_t is_end_of_body;
    uint16_t flags;
    uint16_t phone_book_size;
    uint8_t  new_missed_calls;
    uint8_t  primary_version_counter[16];
    uint8_t  secondary_version_counter[16];
    uint8_t  database_id[16];
};
// header for a single vCard
struct PBAPC_HEADER_VCARD {
    uint8_t device_id;
    uint8_t is_end_of_body;
    uint16_t flags;
    uint8_t  database_id[16];
};
// header for a supported features event
struct PBAPC_HEADER_FEATURES {
    uint8_t device_id;
    uint8_t supported_repos;
    uint8_t supported_features[4];
    uint8_t profile_version_major;
    uint8_t profile_version_minor;
};
// the combined event header
struct PBAPC_EVENT_HEADER {
    uint8_t event_code;
    union {
        struct PBAPC_HEADER_ID event_idOnly;
        struct PBAPC_HEADER_ID_STATUS event_idStatus;
        struct PBAPC_HEADER_PHONEBOOK event_phonebook;
        struct PBAPC_HEADER_VCARD event_vcard;
        struct PBAPC_HEADER_FEATURES event_features;
    };
};


/**
 * debugEntries
 * - used to output the phone book to the debug console
 */
static void debugEntries() {
    LogDebug(LOG_SOURCE_BT, "Phone Book Entries");
    for (int i = 0; i < PHONE_BOOK_MAX_ENTRIES; i++) {
        LogRawDebug(LOG_SOURCE_BT, "[%d] %s", i, _phonebook[i].name ? _phonebook[i].name : "(null)");
        for (int j = 0; j < PHONE_BOOK_MAX_NUMBERS; j++) {
            if (_phonebook[i].phone[j].number[0] != 0) {
                LogRawDebug(LOG_SOURCE_BT, ", %s:%s", _phonebook[i].phone[j].type, _phonebook[i].phone[j].number);
            }
        }
        LogRawDebug(LOG_SOURCE_BT, "\r\n");
    }
}

/**
 * parseVCards
 * a very simple parser to read a few key fields from the returned vCards
 * Expects the buffer (_pb_buffer) to be a UTF-8 string that meets vCard spec.
 */
void parseVCards() 
{
    LogDebug(LOG_SOURCE_BT, "Parsing vCard Buffer...");
//    LogDebug(LOG_SOURCE_BT, "%s", _pb_buffer);

    // break the buffer into lines
    char *lptr;
    char *l = strtok_r((char *)_pb_buffer, "\r\n", &lptr);
    uint8_t idx = 0;
    uint8_t jdx = 0;
    
    struct PHONEBOOK_ENTRY_TYPE entry;
    
    while(l != NULL) {
        char *tptr;
        char *prop = strtok_r(l, ":", &tptr);
        char *prop0 = strtok(prop, ";");
        char *prop1 = strtok(NULL, ";");
        char *value = strtok_r(NULL, ":", &tptr);
        
        if ( UtilsStricmp(prop, "BEGIN") == 0 && 
             UtilsStricmp(value, "VCARD") == 0 ) {
            // BEGIN:VCARD
            // clear the old entry at this index...
            memset(&entry, 0, sizeof(struct PHONEBOOK_ENTRY_TYPE));
            jdx = 0;
        }
        else if (UtilsStricmp(prop, "END") == 0 
            && UtilsStricmp(value, "VCARD") == 0)
        {
            // save it only if there is at least a name and a number
            if (strlen(entry.name) > 0 && strlen(entry.phone[0].number) > 0) {
                memcpy(&(_phonebook[idx]), &entry, sizeof(struct PHONEBOOK_ENTRY_TYPE));
                idx++;
            } else {
                LogDebug(LOG_SOURCE_BT, "- Missing data name=%s phone0=%s", entry.name, entry.phone[0].number);
            }
            if (idx > PHONE_BOOK_MAX_ENTRIES) {
                LogWarning("Too many phone book entries (%d > %d)", idx, PHONE_BOOK_MAX_ENTRIES);
                break;
            }
        }
//        else if (UtilsStricmp(prop, "VERSION") == 0) {
//            LogDebug(LOG_SOURCE_BT, "  VERSION: %s", value);
//        }
        else if (UtilsStricmp(prop, "FN") == 0) {
            // FN: Formatted Name - use this one
            strncpy(entry.name, value, BT_CALLER_ID_FIELD_SIZE - 1);
        }
        else if (UtilsStricmp(prop0, "X-IRMC-CALL-DATETIME") == 0) {
            LogDebug(LOG_SOURCE_BT, "Call %s at: %s", prop1, value);
        }
        else if (UtilsStricmp(prop0, "TEL") == 0) {
            if (prop1) {
                char *eq = strstr(prop1, "=");
                if (eq) prop1 = (eq + 1);
            }

            if (jdx < PHONE_BOOK_MAX_ENTRIES) {
                if (value) strncpy(entry.phone[jdx].number, value, BT_CALLER_ID_FIELD_SIZE - 1);
                if (prop1) {
                    strncpy(entry.phone[jdx].type, prop1, BT_CALLER_ID_FIELD_SIZE - 1);
                } else {
                    strncpy(entry.phone[jdx].type, DEFAULT_TEL_TYPE_NAME, BT_CALLER_ID_FIELD_SIZE - 1);
                }
                jdx++;
            } else {
                LogWarning("Too many phone numbers (> %d)", PHONE_BOOK_MAX_NUMBERS );
            }
        }

        l = strtok_r(NULL, "\r\n", &lptr);
    }

    // DEBUG
    debugEntries();
    
    // TODO - Send an event for anyone who wants to know?
    // EventTriggerCallback(BT_EVENT_PHONEBOOK_UPDATE)
}


/**
 * BM83CommandPBAPOpenSession
 * @param bt - the Bluetooth module
 * 
 * Requests to open a phone book session (if not already open)
 */
void BM83CommandPBAPOpenSession(BT_t *bt)
{
    if (_pb_status != Disconnected) return;
    
    uint8_t command[] = {
        BM83_CMD_PBAPC_CMD,
        BM83_PBAP_OP_OPEN_SESSION,
        bt->activeDevice.deviceId & 0xF // Linked Database, the lower nibble
    };
    BM83SendCommand(bt, command, sizeof(command));
}

/**
 * BM83CommandPBAPCloseSession
 * @param bt - the Bluetooth module
 * 
 * Requests to close the phone book session (if open)
 */
void BM83CommandPBAPCloseSession(BT_t *bt)
{
    if (_pb_status == Disconnected) return;
    
    uint8_t command[] = {
        BM83_CMD_PBAPC_CMD,
        BM83_PBAP_OP_CLOSE_SESSION,
        bt->activeDevice.deviceId & 0xF // Linked Database, the lower nibble
    };
    BM83SendCommand(bt, command, sizeof(command));
}

/**
 * BM83CommandPBAPAbort
 * @param bt - the Bluetooth module
 * 
 * sends the abort command (to stop a running phone book operation)
 */
void BM83CommandPBAPAbort(BT_t *bt)
{
    if (_pb_status == Disconnected) return;
    
    uint8_t command[] = {
        BM83_CMD_PBAPC_CMD,
        BM83_PBAP_OP_ABORT,
        bt->activeDevice.deviceId & 0xF // Linked Database, the lower nibble
    };
    BM83SendCommand(bt, command, sizeof(command));
}


//
// Packet header parsing...
//
void parseIdHeader(struct PBAPC_HEADER_ID *header, uint8_t *data)
{
    header->device_id = *(data);
}
void parseIdStatusHeader(struct PBAPC_HEADER_ID_STATUS *header, uint8_t *data)
{
    header->device_id = data[0];
    header->status = data[1];
}
void parsePhonebookHeader(struct PBAPC_HEADER_PHONEBOOK *header, uint8_t *data)
{
    header->device_id = data[0];
    header->is_end_of_body = data[1];
    header->flags = (data[2] << 8) + data[3];
    if (header->flags & FLAGS_PB_SIZE) {
        header->phone_book_size = (data[4] << 8) + data[5];
    } else {
        header->phone_book_size = 0;
    }
    header->new_missed_calls = data[6];
    if (header->flags & FLAGS_PB_PRIMARY_VER) {
        memcpy(header->primary_version_counter, data + 7, 16);
    } else {
        memset(header->primary_version_counter, 0, 16);
    }
    if (header->flags & FLAGS_PB_SECONDARY_VER) {
        memcpy(header->secondary_version_counter, data + 23, 16);
    } else {
        memset(header->secondary_version_counter, 0, 16);
    }
    if (header->flags & FLAGS_PB_DATABASE_ID) {
        memcpy(header->database_id, data + 39, 16);
    } else {
        memset(header->database_id, 0, 16);
    }
}
void parseVCardHeader(struct PBAPC_HEADER_VCARD *header, uint8_t *data)
{
    header->device_id = data[0];
    header->is_end_of_body = data[1];
    header->flags = (data[2] << 8) + data[3];
    if (header->flags & 0x0400) {
        memcpy(header->database_id, data + 4, 16);
    } else {
        memset(header->database_id, 0, 16);
    }   
}
void parseFeaturesHeader(struct PBAPC_HEADER_FEATURES *header, uint8_t *data)
{
    header->device_id = data[0];
    header->supported_repos = data[1];
    memcpy(header->supported_features, data+2, 4);
    header->profile_version_major = data[6];
    header->profile_version_minor = data[7];
}


//
// Buffer management
//
static void clearBuffer()
{
    memset(_pb_buffer, 0, PHONE_BOOK_MAX_BUFFER);
    _pb_buffer_offset = 0;
}
static int appendBuffer(uint8_t *data, size_t size) 
{
    if (_pb_buffer_offset + size < PHONE_BOOK_MAX_BUFFER) {
        memcpy(_pb_buffer + _pb_buffer_offset, data, size);
        _pb_buffer_offset += size;
    } else {
        return -1;
    }
    
    return 0;
}

/**
 * BM83CommandPBAPPullPhoneBook
 * @param bt - the bluetooth module
 * @param type - the type of phone book (e.g. PBAP_TYPE_PHONEBOOK)
 * @param offset - the starting offset
 * 
 * sends a command to fetch the next set of phonebook entries
 */
void BM83CommandPBAPPullPhoneBook(BT_t *bt, uint8_t type, uint16_t offset)
{
    if (_pb_status == Disconnected) {
        // add this to an "onConnect"
        _pb_onConnect.isValid = 1;
        _pb_onConnect.type = type;
        _pb_onConnect.offset = offset;
        
        BM83CommandPBAPOpenSession(bt);
        return;
    } 
    if (!_pb_status == Connected) {
        LogInfo(LOG_SOURCE_BT, "Attempted to get a phone book without a connection");
        return;
    }
    
    uint8_t command[] = {
        BM83_CMD_PBAPC_CMD,
        BM83_PBAP_OP_PULL_PHONEBOOK,
        bt->activeDevice.deviceId & 0xF, // Linked Database, the lower nibble
        0x00,   // repository 0x00 = Phone, 0x01 = SIM
        type,   // object type (1 byte))
        0x40,   // Flags (2 bytes): b14 = max_list_count
        0x07,   // Flags (2 bytes): b0 = properties, b1 = format, b2 = offset
        (PHONE_BOOK_MAX_ENTRIES & 0xFF00) >> 8,   // max entries (2 bytes))
        (PHONE_BOOK_MAX_ENTRIES & 0xFF),
        0x87,   // Property selector (8 bytes) - tel, fn, n, version
        0x00,   
        0x00,   
        0x00,
        0x00,
        0x00,
        0x00,   
        0x00,   
        0x01,   // Format (1 byte): 0x00 = vCard2.1, 0x01 = vCard3.0
        (offset & 0xFF00) >> 8, // List start offset: (2 bytes))
        (offset & 0xFF),
        0x00,   // vCard selector (8 bytes) - not used
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00    // vCard selector operator (1 byte) - not used
    };
    
    _pb_status = Waiting;
    BM83SendCommand(bt, command, sizeof(command));
}
void BM83CommandPBAPContPhonebook(BT_t *bt) {
    uint8_t command[] = {
        BM83_CMD_PBAPC_CMD,
        BM83_PBAP_OP_PULL_PHONEBOOK,
        bt->activeDevice.deviceId & 0xF, // Linked Database, the lower nibble
        0x00,   // repository 0x00 = Phone, 0x01 = SIM
        0x00,   // object type (1 byte)
        0x00,   // Flags (2 bytes): ignore for continue)
        0x00,   
        0x00,   // Max entries (2 bytes): ignored
        0x00,
        0x00,   // Property selector (8 bytes): ignore
        0x00,   
        0x00,   
        0x00,
        0x00,
        0x00,
        0x00,   
        0x00,   
        0x01,   // Format (1 byte): 0x00 = vCard2.1, 0x01 = vCard3.0
        0x00,   // List start offset (2 bytes): ignore
        0x00,
        0x00,   // vCard selector (8 bytes) - not used
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00    // vCard selector operator (1 byte) - not used
    };
    BM83SendCommand(bt, command, sizeof(command));
}


/**
 * BM83ProcessEventPhoneBook
 * @param bt - the bluetooth module
 * @param data - message bytes
 * @param length - message length
 * 
 * Phonebook events may be fragmented and spread across multiple messages.
 * Fragmented messages are assembled into the buffer
 * Full messages are processed from the buffer.
 * If they are not a "last" message, then strip the headers and keep the data
 * Once all vcard messages are received, then process the vCards in the buffer.
 * 
 */
void BM83ProcessEventPhoneBook(BT_t *bt, uint8_t *data, uint16_t length)
{    
    uint8_t event_type = data[0];
    uint16_t total_length = (data[1] << 8) + data[2];
    uint16_t payload_length = (data[3] << 8) + data[4];

    LogDebug(LOG_SOURCE_BT, "PBAP Packet: Type=%x Length=%d, Payload=%d",
        event_type, total_length, payload_length
    );
    
    static uint16_t payload_start = 0;
    static struct PBAPC_EVENT_HEADER event;

    // First fragment - make sure we can fit the full fragment
    // and store it if we can
    if (event_type == BM83_PBAP_EVENT_TYPE_FRAGMENT_START) {
        _pb_status = Waiting;
        
        if (_pb_buffer_offset + total_length + 1 > PHONE_BOOK_MAX_BUFFER) {
            LogWarning("BT PBAP - Not enough memory for packet (%d)",
                total_length
            );
            BM83CommandPBAPAbort(bt);
        }
        else if (length < 6 ||
            (data[5] == BM83_PBAP_EVENT_PULL_PHONEBOOK_RSP && length < 56))
        {
            LogWarning("BT PBAP - truncated packet received");
            BM83CommandPBAPAbort(bt);
        }
        else if (data[5] != BM83_PBAP_EVENT_PULL_PHONEBOOK_RSP) {
            LogWarning("BT Phonebook - opcode %x with fragments is not supported",
                data[5]
            );
            BM83CommandPBAPAbort(bt);
        }
        else 
        {
            // we are receiving a multi-fragment phonebook response
            // this is the first part - save the header ...
            event.event_code = data[5];
            parsePhonebookHeader(&(event.event_phonebook), data + 6);
            
            // ... and start filling the vcard buffer with the data...
            if (payload_length > 61) {
                payload_start = _pb_buffer_offset;
                appendBuffer((data+61),(payload_length-56));
            }
            
            LogDebug(LOG_SOURCE_BT, 
                "+ First %d data bytes: %d of %d", 
                payload_length,
                payload_length,
                total_length
             ); 
        }
        return;
    }

    if (event_type == BM83_PBAP_EVENT_TYPE_FRAGMENT_CONTINUE) {
        if (payload_start + total_length + 1 > PHONE_BOOK_MAX_BUFFER) {
            LogDebug(LOG_SOURCE_BT, "  Skipped %d bytes...", payload_length);
        } else {
            appendBuffer((data + 5), payload_length);
            LogDebug(LOG_SOURCE_BT, "+ Added %d bytes to buffer: %d of %d",
                payload_length, _pb_buffer_offset - payload_start + 56, total_length); 
        }
        return;
    }

    if (event_type == BM83_PBAP_EVENT_TYPE_FRAGMENT_END) {
        if (payload_start + total_length + 1 > PHONE_BOOK_MAX_BUFFER) {
            LogDebug(LOG_SOURCE_BT, "  Skipped %d bytes...", payload_length);

            // the fragment is finished - but we couldn't buffer it
            // reset for the next try...
            clearBuffer();
            return;
        }
        else
        {
            appendBuffer((data+5), payload_length);
            _pb_buffer[_pb_buffer_offset] = 0;
            LogDebug(LOG_SOURCE_BT, "+ Final %d bytes to buffer: %d of %d",
                payload_length, _pb_buffer_offset - payload_start + 56, total_length);
        }
    } 

    if (event_type == BM83_PBAP_EVENT_TYPE_SINGLE) { 
        _pb_status = Connected;
                
        event.event_code = data[5];
        switch (event.event_code) {
            case BM83_PBAP_EVENT_CONNECTED:
            case BM83_PBAP_EVENT_ERROR_RSP:
                parseIdStatusHeader(&(event.event_idStatus), data + 6);
                break;
            case BM83_PBAP_EVENT_DISCONNECTED:
            case BM83_PBAP_EVENT_SET_PHONEBOOK_RSP:
            case BM83_PBAP_EVENT_ABORT_RSP:        
                parseIdHeader(&(event.event_idOnly), data + 6);
                break;
            case BM83_PBAP_EVENT_PULL_PHONEBOOK_RSP:
            case BM83_PBAP_EVENT_PULL_VCARD_LIST_RSP:
                parsePhonebookHeader(&(event.event_phonebook), data + 6);
                // copy the data body into the buffer
                payload_start = _pb_buffer_offset;
                if (appendBuffer(data + 61, payload_length - 56) < 0) {
                    LogDebug(LOG_SOURCE_BT, "  Not enough memory. Skipping packet");
                    // dump the buffer and start again...
                    clearBuffer();
                    payload_start = 0;
                    return;
                }
                break;
            case BM83_PBAP_EVENT_PULL_VCARD_ENTRY_RSP:
                parseVCardHeader(&(event.event_vcard), data + 6);
                // copy the data body into the buffer
                payload_start = _pb_buffer_offset;
                if (appendBuffer(data + 19, payload_length - 19) < 0) {
                    LogDebug(LOG_SOURCE_BT, "  Not enough memory. Skipping packet");
                    // dump the buffer and start again...
                    clearBuffer();
                    payload_start = 0;
                    return;
                }
                break;
            case BM83_PBAP_EVENT_SUPPORTED_FEATURES:
                parseFeaturesHeader(&(event.event_features), data + 6);
                break;
            default:
                LogWarning("BT Phone Book Unknown message type %x",
                    event.event_code
                );
        }
    }
        
    // respond to the data packet (handles both assembled and single packets)
    switch(event.event_code) {
        case BM83_PBAP_EVENT_CONNECTED:
            if (event.event_idStatus.status != 0) {
                LogInfo(LOG_SOURCE_BT, "PBAP Session connection error %x",
                   event.event_idStatus.status
                 );
                _pb_status = Disconnected;
                clearBuffer();
            } else {
                LogDebug(LOG_SOURCE_BT, "PBAP Session connection success (%x)", 
                    event.event_idStatus.status
                );
                _pb_status = Connected;
                clearBuffer();

                // Make an onConnect command (if needed)
                if (_pb_onConnect.isValid == 1) {
                    _pb_onConnect.isValid = 0;  // don't do it again
                    BM83CommandPBAPPullPhoneBook(bt, _pb_onConnect.type, _pb_onConnect.offset);
                }
                
            }
            break;
            
        case BM83_PBAP_EVENT_DISCONNECTED:
            LogDebug(LOG_SOURCE_BT, "BT: PBAP Session disconnected");
            _pb_status = Disconnected;
            clearBuffer();
            break;
            
        case BM83_PBAP_EVENT_PULL_PHONEBOOK_RSP:
            LogDebug(
                LOG_SOURCE_BT, 
                "Received Phone Book Response: deviceId=%d, isEndOfBody=%s, flags=%04x",
                event.event_phonebook.device_id,
                (event.event_phonebook.is_end_of_body == 0) ? "0 [more to come]" : "1 [last packet]",
                event.event_phonebook.flags
            );

            if ((event.event_phonebook.flags & FLAGS_PB_SIZE) > 0) { 
                LogDebug(
                    LOG_SOURCE_BT, 
                    "Phone Book Size: %d", 
                    event.event_phonebook.phone_book_size
                ); 
            }
            if ((event.event_phonebook.flags & (FLAGS_PB_PRIMARY_VER | FLAGS_PB_SECONDARY_VER)) > 0) { 
                LogRawDebug(
                    LOG_SOURCE_BT, 
                    "Phone Book Versions "
                );
                for (int i = 0; i < 16; i++) {
                    LogRawDebug(LOG_SOURCE_BT, "%02x", event.event_phonebook.primary_version_counter[i]);
                }
                LogRawDebug(LOG_SOURCE_BT, ".");
                for (int i = 0; i < 16; i++) {
                    LogRawDebug(LOG_SOURCE_BT, "%02x", event.event_phonebook.secondary_version_counter[i]);
                }
                LogRawDebug(LOG_SOURCE_BT, "\r\n");
            }
            if ((event.event_phonebook.flags & FLAGS_PB_DATABASE_ID) > 0) { 
                LogRawDebug(
                    LOG_SOURCE_BT, 
                    "Phone Book database ID: "
                );
                for (int i = 0; i < 16; i++) {
                    LogRawDebug(LOG_SOURCE_BT, "LOG_SOURCE_BT, %02x", event.event_phonebook.database_id[i]);
                };
                LogRawDebug(LOG_SOURCE_BT, "\r\n");
            }
            
            if (event.event_phonebook.is_end_of_body == 0) {
                // need to request continuation packets (not documented in BM83)
                // assume this follows Obex (same opcode, final bit, no headers)
                LogDebug(LOG_SOURCE_BT, "More Phone Book data to come...");                
                BM83CommandPBAPContPhonebook(bt);
            }
            if (event.event_phonebook.is_end_of_body == 1) {
                // this is the last packet. Process the vcard data (in the buffer)
                parseVCards();
                clearBuffer();
                EventTriggerCallback(BT_EVENT_PHONEBOOK_UPDATE, 0);
                
                // Disconnect the session now that we have the last chunk
                BM83CommandPBAPCloseSession(bt);
            }
            
            break;
            
        case BM83_PBAP_EVENT_ABORT_RSP:
            // Abort should leave us ready for next command
            _pb_status = Connected;
            clearBuffer();
            break;
            
        case BM83_PBAP_EVENT_ERROR_RSP:
            LogDebug(LOG_SOURCE_BT, "PBAP Error %x", event.event_idStatus.status);
            
            // some event types indicate a connection failure
            // if we see these, assume we are disconnected
            _pb_status = (
                event.event_idStatus.status == 0x48 || 
                event.event_idStatus.status == 0x52 || 
                event.event_idStatus.status == 0x53 ||
                event.event_idStatus.status == 0x54 || 
                event.event_idStatus.status == 0x55
                ) ? Disconnected : Connected;

            clearBuffer();
            break;
            
        case BM83_PBAP_EVENT_SUPPORTED_FEATURES:
            LogDebug(LOG_SOURCE_BT, 
                "PBAP Supported Features: device(%d) %s%s%s%s features(%x%x%x%x) version (%d.%d)",
                event.event_features.device_id,
                (event.event_features.supported_repos & 0x01) > 0 ? "[Local]" : "",
                (event.event_features.supported_repos & 0x02) > 0 ? "[SIM]" : "",
                (event.event_features.supported_repos & 0x04) > 0 ? "[Speed Dial]" : "",
                (event.event_features.supported_repos & 0x08) > 0 ? "[Favorites]" : "",
                event.event_features.supported_features[0],
                event.event_features.supported_features[1],
                event.event_features.supported_features[2],
                event.event_features.supported_features[3],
                event.event_features.profile_version_major,
                event.event_features.profile_version_minor
            );
            _pb_status = Connected;
            clearBuffer();
            break;
            
        default:
            // received an unknown but complete message
            LogDebug(LOG_SOURCE_BT, "PBAP Command %x - not handled", event.event_code);
            _pb_status = Connected;
            clearBuffer();

    }
}

struct PHONEBOOK_ENTRY_TYPE *BM83GetPhonebookEntries() {
    return _phonebook;
}