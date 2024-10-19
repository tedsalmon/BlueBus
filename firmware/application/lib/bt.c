/*
 * File:   bt.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of the abstract Bluetooth Module API
 */
#include "bt.h"
#include "locale.h"
#include "uart.h"

/**
 * BTInit()
 *     Description:
 *         Returns a fresh BT_t object to the caller
 *     Params:
 *         None
 *     Returns:
 *         BT_t *
 */
BT_t BTInit()
{
    BT_t bt;
    memset(&bt, 0, sizeof(BT_t));
    bt.status = BT_STATUS_OFF;
    bt.type = UtilsGetBoardVersion();
    bt.connectable = BT_STATE_ON;
    bt.discoverable = BT_STATE_ON;
    bt.callStatus = BT_CALL_INACTIVE;
    bt.scoStatus = BT_CALL_SCO_CLOSE;
    bt.metadataStatus = BT_METADATA_STATUS_CUR;
    bt.vrStatus = BT_VOICE_RECOG_OFF;
    bt.pairedDevicesCount = 0;
    bt.playbackStatus = BT_AVRCP_STATUS_PAUSED;
    bt.rxQueueAge = 0;
    bt.powerState = BT_STATE_OFF;
    memset(bt.pairedDevices, 0, sizeof(bt.pairedDevices));
    UtilsStrncpy(bt.callerId, LocaleGetText(LOCALE_STRING_VOICE_ASSISTANT), BT_CALLER_ID_FIELD_SIZE);
    memset(bt.dialBuffer, 0, sizeof(bt.dialBuffer));
    memset(bt.pairingErrors, 0, sizeof(bt.pairingErrors));
    // Make sure that we initialize the char arrays to all zeros
    BTClearMetadata(&bt);
    bt.uart = UARTInit(
        BT_UART_MODULE,
        BT_UART_RX_RPIN,
        BT_UART_TX_RPIN,
        BT_UART_RX_PRIORITY,
        BT_UART_TX_PRIORITY,
        UART_BAUD_115200,
        UART_PARITY_NONE
    );
    if (bt.type == BT_BTM_TYPE_BM83) {
        // The BM83 is not pairable by default
        bt.discoverable = BT_STATE_OFF;
    }
    return bt;
}

/**
 * BTCommandCallAccept()
 *     Description:
 *         Accept the incoming call
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandCallAccept(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandCallAnswer(bt);
    } else {
        BM83CommandCallAccept(bt);
    }
}

/**
 * BTCommandCallEnd()
 *     Description:
 *         End the on-going call
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandCallEnd(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandCallEnd(bt);
    } else {
        BM83CommandCallEnd(bt);
    }
}

/**
 * BTCommandDial()
 *     Description:
 *         Dial a number
 *     Params:
 *         BT_t *bt - A pointer to the module object
 *         char *number - number to call
 *         char *name - name to display
 *     Returns:
 *         void
 */
void BTCommandDial(BT_t *bt, const char *number, const char *name)
{
    if (bt->activeDevice.hfpId > 0) {
        if (name != 0x00 && strlen(name) > 0) {
            UtilsStrncpy(bt->callerId,name,BT_CALLER_ID_FIELD_SIZE);
        } else {
            UtilsStrncpy(bt->callerId,number,BT_CALLER_ID_FIELD_SIZE);
        }

        char *cleannum = bt->dialBuffer;
        uint8_t pos = 0;
        while (*number !=0 && pos < BT_DIAL_BUFFER_FIELD_SIZE - 2) {
            char c = *number;
            // ITU-T Recommendation V.250 dial command
            if ((c=='+')||(c==',')||(c=='#')||(c=='*')||(c>='0'&&c<='9')||(c>='A'&&c<='C')||(c>='a'&&c<='c')) {
                cleannum[pos++]=c;
            }
            number++;
        }
        cleannum[pos]=0;
        if (bt->type == BT_BTM_TYPE_BC127) {
            // @FIX
            char command[32];
            snprintf(command, 32, "CALL %d OUTGOING %s", bt->activeDevice.hfpId, cleannum);
            BC127SendCommand(bt, command);
        } else {
            BM83CommandDial(bt, cleannum);
        }
    }
}

/**
 * BTCommandRedial()
 *     Description:
 *         Redial last number as known by phone
 *     Params:
 *         BT_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BTCommandRedial(BT_t *bt)
{
    if (bt->activeDevice.hfpId>0) {
        if (bt->type == BT_BTM_TYPE_BC127) {
            BC127CommandAT(bt,"+BLDN");
        } else {
            BM83CommandRedial(bt);
        }
    }
}

/**
 * BTCommandConnect()
 *     Description:
 *         Open an ACL/A2DP connection to the device given
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandConnect(BT_t *bt, BTPairedDevice_t *dev)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        // Set the MAC ID?
        BC127CommandProfileOpen(bt, "A2DP");
    } else {
        uint8_t profiles = BM83_DATA_LINK_BACK_PROFILES_A2DP;
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
            profiles = BM83_DATA_LINK_BACK_PROFILES_A2DP_HF;
        }
        BM83CommandConnect(bt, dev, profiles);
    }
}

/**
 * BTCommandDisconnect()
 *     Description:
 *         Disconnect the active Bluetooth device
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandDisconnect(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandClose(bt, BT_CLOSE_ALL);
    } else {
        BM83CommandDisconnect(bt, BM83_CMD_DISCONNECT_PARAM_ALL);
    }
}

/**
 * BTCommandGetMetadata()
 *     Description:
 *         Get the current media metadata
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandGetMetadata(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandGetMetadata(bt);
    } else {
        BM83CommandAVRCPGetElementAttributesAll(bt);
    }
}

/**
 * BTCommandList()
 *     Description:
 *         Request a list of previously paired devices
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandList(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandList(bt);
    } else {
        BM83CommandReadPairedDevices(bt);
    }
}

/**
 * BTCommandPause()
 *     Description:
 *         Pause Playback
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandPause(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandPause(bt);
    } else {
        BM83CommandMusicControl(bt, BM83_CMD_ACTION_PAUSE);
    }
}

/**
 * BTCommandPlay()
 *     Description:
 *         Resume Playback
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandPlay(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandPlay(bt);
    } else {
        BM83CommandMusicControl(bt, BM83_CMD_ACTION_PLAY);
    }
}

/**
 * BTCommandPlaybackTrackFastforwardStart()
 *     Description:
 *         Begin fast-forwarding
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandPlaybackTrackFastforwardStart(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandForwardSeekPress(bt);
    } else {
        BM83CommandMusicControl(bt, BM83_CMD_ACTION_FF);
    }
}

/**
 * BTCommandPlaybackTrackFastforwardStop()
 *     Description:
 *         Stop fast-forwarding
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandPlaybackTrackFastforwardStop(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandForwardSeekRelease(bt);
    } else {
        BM83CommandMusicControl(bt, BM83_CMD_ACTION_STOP_FF_RW);
    }
}

/**
 * BTCommandPlaybackTrackRewindStart()
 *     Description:
 *         Begin rewinding
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandPlaybackTrackRewindStart(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandBackwardSeekPress(bt);
    } else {
        BM83CommandMusicControl(bt, BM83_CMD_ACTION_RW);
    }
}

/**
 * BTCommandPlaybackTrackRewindStop()
 *     Description:
 *         Stop rewinding
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandPlaybackTrackRewindStop(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandBackwardSeekRelease(bt);
    } else {
        BM83CommandMusicControl(bt, BM83_CMD_ACTION_STOP_FF_RW);
    }
}

/**
 * BTCommandPlaybackTrackNext()
 *     Description:
 *         Skip to the next track
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandPlaybackTrackNext(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandForward(bt);
    } else {
        BM83CommandMusicControl(bt, BM83_CMD_ACTION_NEXT);
    }
}

/**
 * BTCommandPlaybackTrackPrevious()
 *     Description:
 *         Go back to the previous track
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandPlaybackTrackPrevious(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandBackward(bt);
    } else {
        BM83CommandMusicControl(bt, BM83_CMD_ACTION_PREVIOUS);
    }
}

void BTCommandProfileOpen(BT_t *bt)//, char *)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        //BC127CommandProfileOpen(bt);
    } else {
        //BM83CommandMusicControl(bt, BM83_CMD_ACTION_PREVIOUS);
    }
}

/**
 * BTCommandSetConnectable()
 *     Description:
 *         Set the connectable state of the module
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandSetConnectable(BT_t *bt, uint8_t state)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandBtState(bt, state, bt->discoverable);
    } else {
        if (state == BT_STATE_ON) {
            BM83CommandBTMUtilityFunction(
                bt,
                BM83_CMD_BTM_FUNCTION_DISCO_CONN,
                BM83_CMD_BTM_FUNCTION_PARAM_CONN
            );
        } else {
            BM83CommandBTMUtilityFunction(
                bt,
                BM83_CMD_BTM_FUNCTION_DISCO_CONN,
                BM83_CMD_BTM_FUNCTION_PARAM_NO_CONN
            );
        }
    }
}

/**
 * BTCommandSetDiscoverable()
 *     Description:
 *         Set the Pairing state of the module
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandSetDiscoverable(BT_t *bt, uint8_t state)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127CommandBtState(bt, bt->connectable, state);
    } else {
        if (state == BT_STATE_ON) {
            BM83CommandPairingEnable(bt);
        } else {
            BM83CommandPairingDisable(bt);
        }
    }
}

/**
 * BTCommandStartVoiceRecognition()
 *     Description:
 *         Query the phones assistant
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTCommandToggleVoiceRecognition(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        UtilsStrncpy(bt->callerId, LocaleGetText(LOCALE_STRING_VOICE_ASSISTANT), BT_CALLER_ID_FIELD_SIZE);
        BC127CommandToggleVR(bt);
    } else {
        if (bt->vrStatus == BT_VOICE_RECOG_ON) {
            BM83CommandVoiceRecognitionClose(bt);
            UtilsStrncpy(bt->callerId, LocaleGetText(LOCALE_STRING_VOICE_ASSISTANT), BT_CALLER_ID_FIELD_SIZE);
        } else {
            UtilsStrncpy(bt->callerId, LocaleGetText(LOCALE_STRING_VOICE_ASSISTANT), BT_CALLER_ID_FIELD_SIZE);
            BM83CommandVoiceRecognitionOpen(bt);
        }
    }
}

/**
 * BTHasActiveMacId()
 *     Description:
 *        Check if the Active Device has a MAC ID set
 *     Params:
 *         BT_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
uint8_t BTHasActiveMacId(BT_t *bt)
{
    uint8_t testMac[BT_LEN_MAC_ID] = {0};
    return memcmp(bt->activeDevice.macId, testMac, BT_LEN_MAC_ID);
}

/**
 * BTProcess()
 *     Description:
 *         Process the UART queue for the module
 *     Params:
 *         BT_t *bt - The Bluetooth context
 *     Returns:
 *         void
 */
void BTProcess(BT_t *bt)
{
    if (bt->type == BT_BTM_TYPE_BC127) {
        BC127Process(bt);
    } else {
        BM83Process(bt);
    }
}
