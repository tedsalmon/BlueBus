/* 
 * File: bt_bm83_pbap.h
 * Author: Matt C
 * Comments: Bluetooth PBAP phone book access functions for BM83 chip
 * Revision history: 
 * v0.1 Feb 2024 (Matt C) - initial attempt
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef BT_PHONEBOOK_H
#define	BT_PHONEBOOK_H
#include "bt_common.h"

#define BT_EVENT_PHONEBOOK_UPDATE 18

#define BM83_PBAP_OP_OPEN_SESSION 0x00
#define BM83_PBAP_OP_CLOSE_SESSION 0x01
#define BM83_PBAP_OP_PULL_PHONEBOOK 0x02
#define BM83_PBAP_OP_PULL_LIST 0x03
#define BM83_PBAP_OP_PULL_ENTRY 0x04
#define BM83_PBAP_OP_SET_PHONEBOOK 0x05
#define BM83_PBAP_OP_ABORT 0x06

#define BM83_PBAP_EVENT_TYPE_SINGLE 0x00
#define BM83_PBAP_EVENT_TYPE_FRAGMENT_START 0x01
#define BM83_PBAP_EVENT_TYPE_FRAGMENT_CONTINUE 0x02
#define BM83_PBAP_EVENT_TYPE_FRAGMENT_END 0x03

#define BM83_PBAP_EVENT_CONNECTED 0x00
#define BM83_PBAP_EVENT_DISCONNECTED 0x01
#define BM83_PBAP_EVENT_PULL_PHONEBOOK_RSP 0x02
#define BM83_PBAP_EVENT_PULL_VCARD_LIST_RSP 0x03
#define BM83_PBAP_EVENT_PULL_VCARD_ENTRY_RSP 0x04
#define BM83_PBAP_EVENT_SET_PHONEBOOK_RSP 0x05
#define BM83_PBAP_EVENT_ABORT_RSP 0x06
#define BM83_PBAP_EVENT_ERROR_RSP 0x07
#define BM83_PBAP_EVENT_SUPPORTED_FEATURES 0x08

#define PBAP_TYPE_PHONEBOOK 0x00
#define PBAP_TYPE_INCOMING_CALL_HISTORY 0x01
#define PBAP_TYPE_OUTGOING_CALL_HISTORY 0x02
#define PBAP_TYPE_MISSED_CALL_HISTORY 0x03
#define PBAP_TYPE_COMBINED_CALL_HISTORY 0x04
#define PBAP_TYPE_SPEED_DIAL 0x05
#define PBAP_TYPE_FAVORITE_CONTACTS 0x06

#define PHONE_BOOK_MAX_ENTRIES 8
#define PHONE_BOOK_MAX_NUMBERS 3
#define PHONE_BOOK_MAX_BUFFER 2048

#define FLAGS_PB_SIZE 0x0080
#define FLAGS_PB_PRIMARY_VER 0x0100
#define FLAGS_PB_SECONDARY_VER 0x0200
#define FLAGS_PB_DATABASE_ID 0x0800

#define MIN(a,b) ((a) < (b) ? (a) : (b))

// Phone book structure in-memory
struct PHONE_NUMBER_TYPE {
    char type[BT_CALLER_ID_FIELD_SIZE];
    char number[BT_CALLER_ID_FIELD_SIZE];
};

struct PHONEBOOK_ENTRY_TYPE {
    uint8_t index;
    char name[BT_CALLER_ID_FIELD_SIZE];
    struct PHONE_NUMBER_TYPE phone[PHONE_BOOK_MAX_NUMBERS];
};

void BM83CommandPBAPPullPhoneBook(BT_t *, uint8_t, uint16_t);
struct PHONEBOOK_ENTRY_TYPE *BM83GetPhonebookEntries();

// only exposed for use by CLI
void BM83ProcessEventPhoneBook(BT_t *, uint8_t *, uint16_t);

#endif	/* BT_PHONEBOOK_H */
