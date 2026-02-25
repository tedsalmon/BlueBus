/*
 * File: ibus.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     This implements the I-Bus
 */
#include "ibus.h"
#include "config.h"
#include <ctype.h>

static const uint8_t IBUS_SES_NAV_ZOOM_CONSTANT[IBUS_SES_ZOOM_LEVELS] = {
    0x01, // 125 - special case when stationary
    0x01, // 125 yd 100m
    0x02, // 250 yd 200m
    0x04, // 450 yd 500m
    0x10, // 900 yd 1km
    0x11, // 1 mi 2km
    0x12, // 2.5 mi 5km
    0x13  // 5 mi 10km
};

static const uint8_t IBUS_DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/**
 * IBusInit()
 *     Description:
 *         Returns a fresh IBus_t object to the caller
 *     Params:
 *         None
 *     Returns:
 *         IBus_t*
 */
IBus_t IBusInit()
{
    IBus_t ibus;
    memset(&ibus, 0, sizeof(IBus_t));
    ibus.uart = UARTInit(
        IBUS_UART_MODULE,
        IBUS_UART_RX_RPIN,
        IBUS_UART_TX_RPIN,
        IBUS_UART_RX_PRIORITY,
        IBUS_UART_TX_PRIORITY,
        UART_BAUD_9600,
        UART_PARITY_EVEN
    );
    ibus.cdChangerFunction = IBUS_CDC_FUNC_NOT_PLAYING;
    ibus.ignitionStatus = IBUS_IGNITION_OFF;
    ibus.gtVersion = ConfigGetNavType();
    ibus.vehicleType = ConfigGetVehicleType();
    ibus.lmVariant = ConfigGetLMVariant();
    ibus.lmDimmerVoltage = 0xFF;
    ibus.lmPhotoVoltage = 0xFF; // Photosensor voltage (LSZ)
    ibus.oilTemperature = 0x00;
    memset(ibus.ambientTemperatureCalculated, 0, 7);
    memset(ibus.telematicsLocale, 0, sizeof(ibus.telematicsLocale));
    memset(ibus.telematicsStreet, 0, sizeof(ibus.telematicsStreet));
    memset(ibus.telematicsLatitude, 0, sizeof(ibus.telematicsLatitude));
    memset(ibus.telematicsLongtitude, 0, sizeof(ibus.telematicsLongtitude));
    // Instantiate all our sensors to a value of 255 / 0xFF by default
    IBusPDCSensorStatus_t pdcSensors;
    memset(&pdcSensors, IBUS_PDC_DEFAULT_SENSOR_VALUE, sizeof(pdcSensors));
    ibus.pdcSensors = pdcSensors;
    ibus.txLastStamp = TimerGetMillis();
    return ibus;
}

/**
 * IBusHandleModuleStatus()
 *     Description:
 *         Track module status as we get them and broadcast the event
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t module - The module
 *     Returns:
 *         void
 */
static void IBusHandleModuleStatus(IBus_t *ibus, uint8_t module)
{
    uint8_t detectedModule = IBUS_DEVICE_LOC;
    if (module == IBUS_DEVICE_DSP && ibus->moduleStatus.DSP == 0) {
        ibus->moduleStatus.DSP = 1;
        LogInfo(LOG_SOURCE_IBUS, "DSP Detected");
        detectedModule = IBUS_DEVICE_DSP;
    } else if (module == IBUS_DEVICE_BMBT && ibus->moduleStatus.BMBT == 0) {
        ibus->moduleStatus.BMBT = 1;
        LogInfo(LOG_SOURCE_IBUS, "BMBT Detected");
        detectedModule = IBUS_DEVICE_BMBT;
    } else if (module == IBUS_DEVICE_GT && ibus->moduleStatus.GT == 0) {
        ibus->moduleStatus.GT = 1;
        LogInfo(LOG_SOURCE_IBUS, "GT Detected");
        detectedModule = IBUS_DEVICE_GT;
    } else if (module == IBUS_DEVICE_NAVE && ibus->moduleStatus.NAV == 0) {
        ibus->moduleStatus.NAV = 1;
        LogInfo(LOG_SOURCE_IBUS, "NAV Detected");
        detectedModule = IBUS_DEVICE_NAVE;
    } else if (module == IBUS_DEVICE_VM && ibus->moduleStatus.VM == 0) {
        ibus->moduleStatus.VM = 1;
        LogInfo(LOG_SOURCE_IBUS, "VM Detected");
        detectedModule = IBUS_DEVICE_VM;
    } else if (module == IBUS_DEVICE_LCM && ibus->moduleStatus.LCM == 0) {
        LogInfo(LOG_SOURCE_IBUS, "LCM Detected");
        ibus->moduleStatus.LCM = 1;
        detectedModule = IBUS_DEVICE_LCM;
    } else if (module == IBUS_DEVICE_MID && ibus->moduleStatus.MID == 0) {
        ibus->moduleStatus.MID = 1;
        LogInfo(LOG_SOURCE_IBUS, "MID Detected");
        detectedModule = IBUS_DEVICE_MID;
    } else if (module == IBUS_DEVICE_PDC && ibus->moduleStatus.PDC == 0) {
        ibus->moduleStatus.PDC = 1;
        LogInfo(LOG_SOURCE_IBUS, "PDC Detected");
        detectedModule = IBUS_DEVICE_PDC;
    } else if (module == IBUS_DEVICE_IRIS && ibus->moduleStatus.IRIS == 0) {
        ibus->moduleStatus.IRIS = 1;
        LogInfo(LOG_SOURCE_IBUS, "IRIS Detected");
        detectedModule = IBUS_DEVICE_IRIS;
    } else if (module == IBUS_DEVICE_RAD && ibus->moduleStatus.RAD == 0) {
        ibus->moduleStatus.RAD = 1;
        LogInfo(LOG_SOURCE_IBUS, "RAD Detected");
        detectedModule = IBUS_DEVICE_RAD;
    } else if (module == IBUS_DEVICE_GM && ibus->moduleStatus.GM == 0) {
        ibus->moduleStatus.GM = 1;
        LogInfo(LOG_SOURCE_IBUS, "GM Detected");
        detectedModule = IBUS_DEVICE_GM;
    }
    if (detectedModule != IBUS_DEVICE_LOC) {
        EventTriggerCallback(IBUS_EVENT_MODULE_STATUS_RESP, &detectedModule);
    }
}

/**
 * IBusHandleBlueBusMessage()
 *     Description:
 *         Handle any messages received from the BlueBus (Masquerading as the CDC)
 *     Params:
 *         uint8_t *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleBlueBusMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_BLUEBUS_CMD_SET_STATUS) {
        if (pkt[IBUS_PKT_DB1] == IBUS_BLUEBUS_SUBCMD_SET_STATUS_TEL) {
            EventTriggerCallback(IBUS_EVENT_BLUEBUS_TEL_STATUS_UPDATE, pkt);
        }
    }
}

/**
 * IBusHandleBMBTMessage()
 *     Description:
 *         Handle any messages received from the BMBT (Board Monitor)
 *     Params:
 *         uint8_t *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleBMBTMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_BMBT_BUTTON0 ||
        pkt[IBUS_PKT_CMD] == IBUS_CMD_BMBT_BUTTON1
    ) {
        EventTriggerCallback(IBUS_EVENT_BMBT_BUTTON, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_VOL_CTRL) {
        EventTriggerCallback(IBUS_EVENT_RAD_VOLUME_CHANGE, pkt);
    }
}

/**
 * IBusHandleDSPMessage()
 *     Description:
 *         Handle any messages received from the DSP Amplifier
 *     Params:
 *         uint8_t *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleDSPMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (ibus->moduleStatus.DSP == 0) {
        IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    }
}

/**
 * IBusHandleEWSMessage()
 *     Description:
 *         Handle any messages received from the EWS
 *     Params:
 *         uint8_t *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleEWSMessage(IBus_t *ibus, uint8_t *pkt)
{
    // Do nothing for now -- for future use
}

/**
 * IBusHandleGMMessage()
 *     Description:
 *         Handle any messages received from the GM (Body Module)
 *     Params:
 *         uint8_t *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleGMMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GM_DOORS_FLAPS_STATUS_RESP) {
        EventTriggerCallback(IBUS_EVENT_DOORS_FLAPS_STATUS_RESPONSE, pkt);
    } else if (pkt[IBUS_PKT_CMD] == 0xB0) {
        uint8_t err = IBUS_GM_IDENT_ERR;
        EventTriggerCallback(IBUS_EVENT_GM_IDENT_RESP, &err);
    } else if (
        pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE &&
        pkt[IBUS_PKT_LEN] == 0x0F
    ) {
        uint8_t diagnosticIdx = pkt[10];
        uint8_t moduleVariant = 0x00;
        LogRaw("\r\nIBus: GM DI: %02X\r\n", diagnosticIdx);
        if (diagnosticIdx < 0x20) {
            LogInfo(LOG_SOURCE_IBUS, "GM: ZKE4");
            moduleVariant = IBUS_GM_ZKE4;
        }
        switch (diagnosticIdx) {
            case 0x20:
            case 0x21:
            case 0x22:
                LogInfo(LOG_SOURCE_IBUS, "GM: ZKE3_GM1");
                moduleVariant = IBUS_GM_ZKE3_GM1;
                break;
            case 0x25:
                LogInfo(LOG_SOURCE_IBUS, "GM: ZKE3_GM5");
                moduleVariant = IBUS_GM_ZKE3_GM5;
                break;
            case 0x40:
            case 0x41:
            case 0x42:
            case 0x50:
            case 0x51:
            case 0x52:
                LogInfo(LOG_SOURCE_IBUS, "GM: ZKE5");
                moduleVariant = IBUS_GM_ZKE5;
                break;
            case 0x45:
            case 0x46:
            case 0x55:
            case 0x56:
                LogInfo(LOG_SOURCE_IBUS, "GM: ZKE5_S12");
                moduleVariant = IBUS_GM_ZKE5_S12;
                break;
            case 0x80:
            case 0x81:
                LogInfo(LOG_SOURCE_IBUS, "GM: ZKE3_GM4");
                moduleVariant = IBUS_GM_ZKE3_GM4;
                break;
            case 0x85:
                LogInfo(LOG_SOURCE_IBUS, "GM: ZKE3_GM6");
                moduleVariant = IBUS_GM_ZKE3_GM6;
                break;
            case 0xA0:
                LogInfo(LOG_SOURCE_IBUS, "GM: ZKEBC1");
                moduleVariant = IBUS_GM_ZKEBC1;
                break;
            case 0xA3:
                LogInfo(LOG_SOURCE_IBUS, "GM: ZKEBC1RD");
                moduleVariant = IBUS_GM_ZKEBC1RD;
                break;
        }
        EventTriggerCallback(IBUS_EVENT_GM_IDENT_RESP, &moduleVariant);
    }
    // Any GM (ZKE) Traffic should trigger the module status update
    if (ibus->moduleStatus.GM == 0) {
        IBusHandleModuleStatus(ibus, IBUS_DEVICE_GM);
    }
}

/**
 * IBusHandleGTMessage()
 *     Description:
 *         Handle any messages received from the GT (Graphics Terminal)
 *     Params:
 *         uint8_t *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleGTMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
         IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    } else if (pkt[IBUS_PKT_LEN] == 0x22 &&
        pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
        pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE
    ) {
        // Decode the software and hardware versions
        uint8_t hardwareVersion = IBusGetNavHWVersion(pkt);
        uint8_t softwareVersion = IBusGetNavSWVersion(pkt);
        uint8_t diagnosticIndex = IBusGetNavDiagnosticIndex(pkt);
        uint8_t gtVersion = IBusGetNavType(pkt);
        if (gtVersion != IBUS_GT_DETECT_ERROR) {
            LogRaw(
                "\r\nIBus: GT P/N: %c%c%c%c%c%c%c DI: %d HW: %d SW: %d Build: %c%c/%c%c\r\n",
                pkt[IBUS_PKT_DB1],
                pkt[IBUS_PKT_DB2],
                pkt[IBUS_PKT_DB3],
                pkt[IBUS_PKT_DB4],
                pkt[IBUS_PKT_DB5],
                pkt[IBUS_PKT_DB6],
                pkt[10],
                diagnosticIndex,
                hardwareVersion,
                softwareVersion,
                pkt[19],
                pkt[20],
                pkt[21],
                pkt[22]
            );
            ibus->gtVersion = gtVersion;
            EventTriggerCallback(IBUS_EVENT_GT_DIA_IDENTITY_RESPONSE, &gtVersion);
        } else {
            LogError("IBus: Unable to decode navigation type");
        }
    } else if (pkt[IBUS_PKT_LEN] >= 0x0C &&
        pkt[IBUS_PKT_LEN] < 0x22 &&
        pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
        pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE
    ) {
        // Example Frame: 3B 0C 3F A0 42 4D 57 43 30 31 53 00 00 E1
        EventTriggerCallback(IBUS_EVENT_GT_DIA_OS_IDENTITY_RESPONSE, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_MENU_SELECT) {
        EventTriggerCallback(IBUS_EVENT_GT_MENU_SELECT, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_SCREEN_MODE_SET) {
        EventTriggerCallback(IBUS_EVENT_SCREEN_MODE_SET, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_CHANGE_UI_REQ) {
        // Example Frame: 3B 05 FF 20 02 0C EF [Telephone Selected]
        EventTriggerCallback(IBUS_EVENT_GT_CHANGE_UI_REQUEST, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_MENU_BUFFER_STATUS) {
        EventTriggerCallback(IBUS_EVENT_GT_MENU_BUFFER_UPDATE, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_BMBT_BUTTON1) {
        // The GT broadcasts an emulated version of the BMBT button press
        // command 0x48 that matches the "Phone" button on the BMBT
        EventTriggerCallback(IBUS_EVENT_BMBT_BUTTON, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_RAD_AUDIO_INPUT) {
        EventTriggerCallback(IBUS_EVENT_GT_AUDIO_INPUT_CONTROL, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_MONITOR_CONTROL) {
        ibus->videoSource = pkt[IBUS_PKT_DB1] & 0x02;
        EventTriggerCallback(IBUS_EVENT_MONITOR_STATUS, pkt);
    }
}

/**
 * IBusHandleIKEMessage()
 *     Description:
 *         Handle any messages received from the IKE (Instrument Cluster)
 *     Params:
 *         uint8_t *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleIKEMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IKE_IGN_STATUS_RESP) {
        uint8_t ignitionStatus = pkt[IBUS_PKT_DB1];
        if (ibus->ignitionStatus != IBUS_IGNITION_KL99) {
            // The order of the items below should not be changed,
            // otherwise listeners will not know if the ignition status
            // has changed
            EventTriggerCallback(
                IBUS_EVENT_IKE_IGNITION_STATUS,
                &ignitionStatus
            );
            ibus->ignitionStatus = ignitionStatus;
        }
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IKE_SENSOR_RESP) {
        ibus->gearPosition = pkt[IBUS_PKT_DB2] >> 4;
        uint8_t valueType = IBUS_SENSOR_VALUE_GEAR_POS;
        EventTriggerCallback(IBUS_EVENT_SENSOR_VALUE_UPDATE, &valueType);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IKE_RESP_VEHICLE_CONFIG) {
        ibus->vehicleType = IBusGetVehicleType(pkt);
        EventTriggerCallback(IBUS_EVENT_IKE_VEHICLE_CONFIG, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IKE_SPEED_RPM_UPDATE) {
        EventTriggerCallback(IBUS_EVENT_IKE_SPEED_RPM_UPDATE, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IKE_TEMP_UPDATE) {
        // Do not update the system if the value is the same
        if (ibus->coolantTemperature != pkt[IBUS_PKT_DB2] && pkt[IBUS_PKT_DB2] <= 0x7F) {
            ibus->coolantTemperature = pkt[IBUS_PKT_DB2];
            uint8_t valueType = IBUS_SENSOR_VALUE_COOLANT_TEMP;
            EventTriggerCallback(IBUS_EVENT_SENSOR_VALUE_UPDATE, &valueType);
        }
        signed char tmp = pkt[IBUS_PKT_DB1];
        if (ibus->ambientTemperature != tmp && tmp > -60 && tmp < 60) {
            ibus->ambientTemperature = tmp;
            uint8_t valueType = IBUS_SENSOR_VALUE_AMBIENT_TEMP;
            EventTriggerCallback(IBUS_EVENT_SENSOR_VALUE_UPDATE, &valueType);
        }
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IKE_OBC_TEXT) {
        char property = pkt[IBUS_PKT_DB1];
        // @TODO: Refactor this
        if (
            property == IBUS_IKE_TEXT_TEMPERATURE &&
            pkt[IBUS_PKT_LEN] >= 7 &&
            pkt[IBUS_PKT_LEN] <= 11
        ) {

            uint8_t *temp = pkt + 6;
            uint8_t size = pkt[IBUS_PKT_LEN] - 5;

            while (size > 0 && temp[0] == ' ') {
                temp++;
                size--;
            }

            if (size > 6) {
                size = 6;
            }

            while (size > 0 && (temp[size-1] == 0x00 || temp[size-1] == ' ' || temp[size - 1] == '.')) {
                size--;
            }

            memset(ibus->ambientTemperatureCalculated, 0, 7);
            memcpy(
                ibus->ambientTemperatureCalculated,
                temp,
                size
            );

            uint8_t valueType = IBUS_SENSOR_VALUE_AMBIENT_TEMP_CALCULATED;
            EventTriggerCallback(IBUS_EVENT_SENSOR_VALUE_UPDATE, &valueType);
        } else if (property == IBUS_IKE_OBC_PROPERTY_TIME) {
            // 15:31,  3:31PM,

            uint8_t hourTens = isdigit(pkt[IBUS_PKT_DB3]) ? pkt[IBUS_PKT_DB3] - '0' : 0;
            uint8_t hourOnes = isdigit(pkt[IBUS_PKT_DB4]) ? pkt[IBUS_PKT_DB4] - '0' : 0;
            uint8_t minTens = isdigit(pkt[IBUS_PKT_DB6]) ? pkt[IBUS_PKT_DB6] - '0' : 0;
            uint8_t minOnes = isdigit(pkt[IBUS_PKT_DB7]) ? pkt[IBUS_PKT_DB7] - '0' : 0;

            ibus->obcDateTime.hour = hourTens * 10 + hourOnes;
            ibus->obcDateTime.min = minTens * 10 + minOnes;
            if (
                (pkt[IBUS_PKT_DB8] == 'P' || pkt[IBUS_PKT_DB8] == 'p') &&
                ibus->obcDateTime.hour < 12
            ) {
                ibus->obcDateTime.hour += 12;
            }
            if (
                (pkt[IBUS_PKT_DB8] == 'A' || pkt[IBUS_PKT_DB8] == 'a') &&
                ibus->obcDateTime.hour == 12
            ) {
                ibus->obcDateTime.hour = 0;
            }
        } else if (property == IBUS_IKE_OBC_PROPERTY_DATE) {
            // 17.01.2020, 01/17/2020, 02.01.2023, --.--.2026
            if (isdigit(pkt[IBUS_PKT_DB9])) {
                ibus->obcDateTime.year = (
                    ((pkt[IBUS_PKT_DB9] - '0') * 1000) +
                    ((pkt[IBUS_PKT_DB10] - '0') * 100) +
                    ((pkt[IBUS_PKT_DB11] - '0') * 10) +
                    (pkt[IBUS_PKT_DB12] - '0')
                );
            }
            uint8_t value1 = 1;
            uint8_t value2 = 1;
            if (isdigit(pkt[IBUS_PKT_DB3]) || isdigit(pkt[IBUS_PKT_DB4])) {
                value1 = (pkt[IBUS_PKT_DB3] - '0' * 10) + pkt[IBUS_PKT_DB4] - '0';
            }
            if (isdigit(pkt[IBUS_PKT_DB6]) || isdigit(pkt[IBUS_PKT_DB7])) {
                value2 = (pkt[IBUS_PKT_DB6] - '0' * 10) + pkt[IBUS_PKT_DB7] - '0';
            }
            if (pkt[IBUS_PKT_DB5] == '/') {
                ibus->obcDateTime.month = value1;
                ibus->obcDateTime.day = value2;
            } else {
                ibus->obcDateTime.month = value2;
                ibus->obcDateTime.day = value1;
            }
        } else if (property == IBUS_IKE_OBC_PROPERTY_RANGE) {
            // "123 KM " or "--- KM " or "123 MLS"
            uint16_t range = 0;
            uint8_t idx = IBUS_PKT_DB3;
            uint8_t maxIdx = IBUS_PKT_DB3 + 4;
            while (idx < maxIdx && isdigit(pkt[idx])) {
                range = range * 10 + (pkt[idx] - '0');
                idx++;
            }
            // Only trigger event if we got a valid numeric range
            if (idx > IBUS_PKT_DB3) {
                ibus->vehicleRange = range;
                uint8_t valueType = IBUS_SENSOR_VALUE_VEHICLE_RANGE;
                EventTriggerCallback(IBUS_EVENT_SENSOR_VALUE_UPDATE, &valueType);
            }
        }
    }
}

/**
 * IBusHandleLCMMessage()
 *     Description:
 *         Handle any messages received from the LCM (Lighting Control Module)
 *     Params:
 *         uint8_t *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleLCMMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_GLO &&
        pkt[IBUS_PKT_CMD] == IBUS_LCM_LIGHT_STATUS_RESP
    ) {
        EventTriggerCallback(IBUS_EVENT_LCM_LIGHT_STATUS, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_GLO &&
               pkt[IBUS_PKT_CMD] == IBUS_LCM_DIMMER_STATUS
    ) {
        EventTriggerCallback(IBUS_EVENT_LCM_DIMMER_STATUS, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE &&
               pkt[IBUS_PKT_LEN] == 0x19
    ) {
        // LME38 has unique status. It's shorter, and different mapping.
        // Length is an (educated) guess based on number of bytes required to
        // populate the job results.
        ibus->lmDimmerVoltage = pkt[IBUS_LME38_IO_DIMMER_OFFSET];
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE &&
               pkt[IBUS_PKT_LEN] == 0x23
    ) {
        // Status reply length and mapping is the same for LCM and LSZ variants.
        // The non-applicable parameters default to 0x00, i.e. LCM does not
        // have a photosensor, so value will be 0x00.

        // Front load sensor voltage (Xenon)
        ibus->lmLoadFrontVoltage = pkt[IBUS_LM_IO_LOAD_FRONT_OFFSET];
        // Dimmer (58G) voltage
        ibus->lmDimmerVoltage = pkt[IBUS_LM_IO_DIMMER_OFFSET];
        // Rear load sensor voltage (Xenon) / manual vertical aim control (non-Xenon)
        ibus->lmLoadRearVoltage = pkt[IBUS_LM_IO_LOAD_REAR_OFFSET];
        // Photosensor voltage (LSZ)
        ibus->lmPhotoVoltage = pkt[IBUS_LM_IO_PHOTO_OFFSET];
        if (ibus->vehicleType != IBUS_VEHICLE_TYPE_E46 &&
            ibus->vehicleType != IBUS_VEHICLE_TYPE_E8X &&
            pkt[23] != 0x00
        ) {
            // Oil Temp calculation
            uint16_t offset = 310;
            if (ibus->lmVariant == IBUS_LM_LCM_IV) {
                offset = 510;
            }
            float rawTemperature = (pkt[23] * 0.00005) + (pkt[24] * 0.01275);
            uint8_t oilTemperature = 67.2529 * log(rawTemperature) + offset;
            if (oilTemperature != ibus->oilTemperature) {
                ibus->oilTemperature = oilTemperature;
                uint8_t valueType = IBUS_SENSOR_VALUE_OIL_TEMP;
                EventTriggerCallback(IBUS_EVENT_SENSOR_VALUE_UPDATE, &valueType);
            }
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE &&
               pkt[IBUS_PKT_LEN] == 0x03
    ) {
        EventTriggerCallback(IBUS_EVENT_LCM_DIAGNOSTICS_ACKNOWLEDGE, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_LCM_RESP_REDUNDANT_DATA) {
        EventTriggerCallback(IBUS_EVENT_LCM_REDUNDANT_DATA, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE &&
               pkt[IBUS_PKT_LEN] == 0x0F
    ) {
      // I was a bit nervous about relying upon message length, but only an
      // ident request (0x00) results in a reply of this length. Winning.
      // I would have done the same GT and had the ident logic in the handler,
      // but it's a bit unwieldy so I split it out.
      uint8_t lmVariant = IBusGetLMVariant(pkt);
      // I did not modify struct in above method as it was not very _SOLID_.
      // Yes, that's right, SOLID. Don't you roll your eyes at me!
      ibus->lmVariant = lmVariant;
      EventTriggerCallback(IBUS_EVENT_LM_IDENT_RESPONSE, &lmVariant);
    }
}

static void IBusHandleMFLMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_MFL_CMD_BTN_PRESS) {
        EventTriggerCallback(IBUS_EVENT_MFL_BUTTON, pkt);
    }
    if (pkt[IBUS_PKT_CMD] == IBUS_MFL_CMD_VOL_PRESS) {
        EventTriggerCallback(IBUS_EVENT_MFL_VOLUME_CHANGE, pkt);
    }
}

static void IBusHandleMIDMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    } else if (
        pkt[IBUS_PKT_DST] == IBUS_DEVICE_RAD ||
        pkt[IBUS_PKT_DST] == IBUS_DEVICE_TEL
    ) {
        if (pkt[IBUS_PKT_CMD] == IBUS_MID_BUTTON_PRESS) {
            EventTriggerCallback(IBUS_EVENT_MID_BUTTON_PRESS, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_LOC) {
        if (pkt[IBUS_PKT_CMD] == IBUS_MID_CMD_MODE) {
            EventTriggerCallback(IBUS_EVENT_MID_MODE_CHANGE, pkt);
        }
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_VOL_CTRL) {
        EventTriggerCallback(IBUS_EVENT_RAD_VOLUME_CHANGE, pkt);
    }
    if (ibus->moduleStatus.MID == 0) {
        IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    }
}

/**
 * IBusHandleNAVMessage()
 *     Description:
 *         Handle any messages received from the non-Jap Navigation Computer
 *     Params:
 *         uint8_t *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleNAVMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_NAV_CMD_GPSTIME) {
        IBusDateTime_t gpsDateTime;
        gpsDateTime.hour = UTILS_UNPACK_8BCD(pkt[IBUS_PKT_DB2]);
        gpsDateTime.min = UTILS_UNPACK_8BCD(pkt[IBUS_PKT_DB3]);
        gpsDateTime.sec = 0;
        gpsDateTime.day = UTILS_UNPACK_8BCD(pkt[IBUS_PKT_DB4]);
        gpsDateTime.month = UTILS_UNPACK_8BCD(pkt[IBUS_PKT_DB6]);
        gpsDateTime.year = (
            UTILS_UNPACK_8BCD(pkt[IBUS_PKT_DB7]) * 100 + UTILS_UNPACK_8BCD(pkt[IBUS_PKT_DB8])
        );
        // Account for GPS rollover issue by adding 1024 weeks worth of seconds
        uint32_t epoch = IBusGetDateTimeAsEpoch(&gpsDateTime) + (1024 * 604800);
        ibus->gpsDateTime = IBusGetEpochAsDateTime(epoch);
        EventTriggerCallback(
            IBUS_EVENT_NAV_GPSDATETIME_UPDATE,
            pkt
        );
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_SCREEN_MODE_SET) {
        EventTriggerCallback(
            IBUS_EVENT_GT_SCREEN_MODE_SET,
            pkt
        );
    }
    // It is imperative that we know if the NAV is on the bus
    // so if we see any message from it, we must consider it "found"
    if (ibus->moduleStatus.NAV == 0) {
        ibus->moduleStatus.NAV = 1;
        uint8_t detectedModule = pkt[IBUS_PKT_SRC];
        EventTriggerCallback(IBUS_EVENT_MODULE_STATUS_RESP, &detectedModule);
    }
}

static void IBusHandlePDCMessage(IBus_t *ibus, uint8_t *pkt)
{
    // The PDC does not seem to handshake via 0x01 / 0x02 so emit this event
    // any time we see 0x5A from the PDC. Keep this above all other code to
    // ensure event listeners know the PDC is alive before performing work
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_LCM_BULB_IND_REQ) {
        IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_PDC_STATUS) {
        EventTriggerCallback(IBUS_EVENT_PDC_STATUS, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_PDC_SENSOR_RESPONSE) {
        // Reinstantiate all our sensors to a value of 255 / 0xFF by default
        IBusPDCSensorStatus_t pdcSensors;
        memset(&pdcSensors, IBUS_PDC_DEFAULT_SENSOR_VALUE, sizeof(pdcSensors));
        ibus->pdcSensors = pdcSensors;
        // Ensure PDC is active -- first bit of the tenth data byte of the packet
        if ((pkt[13] & 0x1) == 1) {
            ibus->pdcSensors.frontLeft = pkt[IBUS_PKT_DB6];
            ibus->pdcSensors.frontCenterLeft = pkt[11];
            ibus->pdcSensors.frontCenterRight = pkt[12];
            ibus->pdcSensors.frontRight = pkt[10];
            ibus->pdcSensors.rearLeft = pkt[IBUS_PKT_DB2];
            ibus->pdcSensors.rearCenterLeft = pkt[IBUS_PKT_DB4];
            ibus->pdcSensors.rearCenterRight = pkt[IBUS_PKT_DB5];
            ibus->pdcSensors.rearRight = pkt[IBUS_PKT_DB3];

            LogDebug(
                LOG_SOURCE_IBUS,
                "PDC distances(cm): F: %i - %i - %i - %i, R: %i - %i - %i - %i",
                ibus->pdcSensors.frontLeft,
                ibus->pdcSensors.frontCenterLeft,
                ibus->pdcSensors.frontCenterRight,
                ibus->pdcSensors.frontRight,
                ibus->pdcSensors.rearLeft,
                ibus->pdcSensors.rearCenterLeft,
                ibus->pdcSensors.rearCenterRight,
                ibus->pdcSensors.rearRight
            );
            EventTriggerCallback(IBUS_EVENT_PDC_SENSOR_UPDATE, pkt);
        }
    }
}

static void IBusHandleRADMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_CDC) {
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_REQ) {
            EventTriggerCallback(IBUS_EVENT_MODULE_STATUS_REQUEST, pkt);
        } else if (pkt[IBUS_PKT_CMD] == IBUS_COMMAND_CDC_REQUEST) {
            if (pkt[IBUS_PKT_DB1] == IBUS_CDC_CMD_STOP_PLAYING) {
                ibus->cdChangerFunction = IBUS_CDC_FUNC_NOT_PLAYING;
            } else if (pkt[IBUS_PKT_DB1] == IBUS_CDC_CMD_PAUSE_PLAYING) {
                ibus->cdChangerFunction = IBUS_CDC_FUNC_PAUSE;
            } else if (pkt[IBUS_PKT_DB1] == IBUS_CDC_CMD_START_PLAYING) {
                ibus->cdChangerFunction = IBUS_CDC_FUNC_PLAYING;
            }
            EventTriggerCallback(IBUS_EVENT_CD_STATUS_REQUEST, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_LEN] > 8 &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE
    ) {
        LogRaw(
            "\r\nIBus: RAD P/N: %d%d%d%d%d%d%d HW: %02d SW: %d%d Build: %d%d/%d%d\r\n",
            pkt[IBUS_PKT_DB1] & 0x0F,
            (pkt[IBUS_PKT_DB2] & 0xF0) >> 4,
            pkt[IBUS_PKT_DB2] & 0x0F,
            (pkt[IBUS_PKT_DB3] & 0xF0) >> 4,
            pkt[IBUS_PKT_DB3] & 0x0F,
            (pkt[IBUS_PKT_DB4] & 0xF0) >> 4,
            pkt[IBUS_PKT_DB4] & 0x0F,
            pkt[IBUS_PKT_DB5],
            (pkt[15] & 0xF0) >> 4,
            pkt[15] & 0x0F,
            (pkt[12] & 0xF0) >> 4,
            pkt[12] & 0x0F,
            (pkt[13] & 0xF0) >> 4,
            pkt[13] & 0x0F
        );
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DSP) {
        if (pkt[IBUS_PKT_CMD] == IBUS_DSP_CMD_CONFIG_SET) {
            EventTriggerCallback(IBUS_EVENT_DSP_CONFIG_SET, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_GT) {
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_RAD_SCREEN_MODE_UPDATE) {
            EventTriggerCallback(IBUS_EVENT_SCREEN_MODE_UPDATE, pkt);
        }
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_RAD_UPDATE_MAIN_AREA) {
            EventTriggerCallback(IBUS_EVENT_RAD_WRITE_DISPLAY, pkt);
        }
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_DISPLAY_RADIO_MENU) {
            EventTriggerCallback(IBUS_EVENT_RAD_DISPLAY_MENU, pkt);
        }
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_WRITE_WITH_CURSOR &&
            pkt[IBUS_PKT_DB2] == 0x01 &&
            pkt[IBUS_PKT_DB3] == 0x00
        ) {
            EventTriggerCallback(IBUS_EVENT_SCREEN_BUFFER_FLUSH, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_IKE) {
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_WRITE_TITLE &&
            pkt[IBUS_PKT_DB1] == 0x41 &&
            pkt[IBUS_PKT_DB2] == 0x30
        ) {
            EventTriggerCallback(IBUS_EVENT_RAD_WRITE_DISPLAY, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_LOC) {
        if (pkt[IBUS_PKT_CMD] == 0x3B) {
            EventTriggerCallback(IBUS_EVENT_CD_CLEAR_DISPLAY, pkt);
        }
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_RAD_UPDATE_MAIN_AREA) {
            EventTriggerCallback(IBUS_EVENT_RAD_WRITE_DISPLAY, pkt);
        }
        if (pkt[IBUS_PKT_CMD] == IBUS_DSP_CMD_CONFIG_SET) {
            EventTriggerCallback(IBUS_EVENT_DSP_CONFIG_SET, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_MID) {
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_RAD_WRITE_MID_DISPLAY) {
            if (pkt[IBUS_PKT_DB1] == 0xC0) {
                EventTriggerCallback(IBUS_EVENT_RAD_MID_DISPLAY_TEXT, pkt);
            }
        } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_RAD_WRITE_MID_MENU) {
            EventTriggerCallback(IBUS_EVENT_RAD_MID_DISPLAY_MENU, pkt);
        }
    }
    EventTriggerCallback(IBUS_EVENT_RAD_MESSAGE_RCV, pkt);
}

static void IBusHandleTELMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_REQ) {
        EventTriggerCallback(IBUS_EVENT_MODULE_STATUS_REQUEST, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_VOL_CTRL) {
        EventTriggerCallback(IBUS_EVENT_TEL_VOLUME_CHANGE, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_TELEMATICS_COORDINATES) {
        // Store latitude and longitude for emergency display
        snprintf(
            ibus->telematicsLatitude,
            IBUS_TELEMATICS_COORDS_LEN,
            "%i\xB0%02X'%02X.%01X\" %c",
            (pkt[IBUS_PKT_DB2] & 0x0F) * 100 + (pkt[IBUS_PKT_DB3] >> 4) * 10 + (pkt[IBUS_PKT_DB3] & 0x0F),
            pkt[IBUS_PKT_DB4],
            pkt[IBUS_PKT_DB5],
            pkt[IBUS_PKT_DB6] >> 4,
            ((pkt[IBUS_PKT_DB6] & 0x01) == 0) ? 'N' : 'S'
        );
        snprintf(
            ibus->telematicsLongtitude,
            IBUS_TELEMATICS_COORDS_LEN,
            "%i\xB0%02X'%02X.%01X\" %c",
            (pkt[10] & 0x0F) * 100 + (pkt[11] >> 4) * 10+ (pkt[11] & 0x0F),
            pkt[12],
            pkt[13],
            pkt[14] >> 4,
            ((pkt[14] & 0x01) == 0) ? 'E': 'W'
        );
        EventTriggerCallback(IBUS_EVENT_GT_TELEMATICS_DATA, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_TELEMATICS_LOCATION) {
        // Store provided location info for emergency display
        pkt[pkt[1] + 1] = 0;
        if (pkt[IBUS_PKT_DB2] == IBUS_DATA_GT_TELEMATICS_LOCALE) {
            UtilsStrncpy(
                ibus->telematicsLocale,
                (char *) pkt + IBUS_PKT_DB3,
                IBUS_TELEMATICS_COORDS_LEN
            );
        } else if (pkt[IBUS_PKT_DB2] == IBUS_DATA_GT_TELEMATICS_STREET) {
            UtilsStrncpy(
                ibus->telematicsStreet,
                (char *) pkt + IBUS_PKT_DB3,
                IBUS_TELEMATICS_COORDS_LEN
            );
            uint8_t len = strlen(ibus->telematicsStreet);
            if (len > 0 && ibus->telematicsStreet[len - 1] == ';') {
                ibus->telematicsStreet[len - 1] = 0;
            }
        }
        EventTriggerCallback(IBUS_EVENT_GT_TELEMATICS_DATA, pkt);
    }
}

static void IBusHandleVMMessage(IBus_t *ibus, uint8_t *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        IBusHandleModuleStatus(ibus, pkt[IBUS_PKT_SRC]);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_RAD_AUDIO_INPUT) {
        EventTriggerCallback(IBUS_EVENT_GT_AUDIO_INPUT_CONTROL, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_MONITOR_CONTROL) {
        ibus->videoSource = pkt[IBUS_PKT_DB1] & 0x02;
        EventTriggerCallback(IBUS_EVENT_MONITOR_STATUS, pkt);
    } else if (
        pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
        pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE &&
        pkt[IBUS_PKT_LEN] >= 0x0F
    ) {
        LogRaw(
            "\r\nIBus: VM P/N: %02X%02X%02X%02X HW: %02X SW: %02X Build: %02X/%02X\r\n",
            pkt[4],
            pkt[5],
            pkt[6],
            pkt[7],
            pkt[9],
            pkt[14],
            pkt[12],
            pkt[13]
        );
        if (ibus->moduleStatus.NAV == 0) {
            ibus->gtVersion = IBUS_GT_MKII;
        }
        EventTriggerCallback(IBUS_EVENT_VM_IDENT_RESP, &ibus->gtVersion);
    }
}

static uint8_t IBusValidateChecksum(uint8_t *msg)
{
    uint8_t chk = 0;
    uint8_t msgSize = msg[1] + 2;
    uint8_t idx;
    for (idx = 0; idx < msgSize; idx++) {
        chk =  chk ^ msg[idx];
    }
    if (chk == 0) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * IBusProcess()
 *     Description:
 *         Process messages in the IBus RX queue
 *     Params:
 *         IBus_t *ibus
 *     Returns:
 *         void
 */
void IBusProcess(IBus_t *ibus)
{
    // Read messages from the IBus and if none are available, attempt to
    // transmit whatever is sitting in the transmit buffer
    if (
        CharQueueGetSize(&ibus->uart.rxQueue) > 0 &&
        ibus->rxBufferIdx < IBUS_RX_BUFFER_SIZE
    ) {
        ibus->rxBuffer[ibus->rxBufferIdx++] = CharQueueNext(&ibus->uart.rxQueue);
        if (ibus->rxBufferIdx > 1) {
            uint8_t msgLength = ibus->rxBuffer[1] + 2;
            // Make sure we do not read more than the maximum packet length
            if (msgLength > IBUS_MAX_MSG_LENGTH) {
                long long unsigned int ts = (long long unsigned int) TimerGetMillis();
                LogRawDebug(
                    LOG_SOURCE_IBUS,
                    "[%llu] ERROR: IBus: RX Invalid Length [%d - %02X]: ",
                    ts,
                    msgLength,
                    ibus->rxBuffer[1]
                );
                uint8_t idx;
                for (idx = 0; idx < ibus->rxBufferIdx; idx++) {
                    LogRawDebug(LOG_SOURCE_IBUS, "%02X ", ibus->rxBuffer[idx]);
                }
                LogRawDebug(LOG_SOURCE_IBUS, "\r\n");
                ibus->rxBufferIdx = 0;
                memset(ibus->rxBuffer, 0, IBUS_RX_BUFFER_SIZE);
                CharQueueReset(&ibus->uart.rxQueue);
            } else if (msgLength == ibus->rxBufferIdx) {
                uint8_t idx;
                uint8_t pkt[msgLength];
                memset(pkt, 0, msgLength);
                long long unsigned int ts = (long long unsigned int) TimerGetMillis();
                LogRawDebug(LOG_SOURCE_IBUS, "[%llu] DEBUG: IBus: RX[%d]: ", ts, msgLength);
                for(idx = 0; idx < msgLength; idx++) {
                    pkt[idx] = ibus->rxBuffer[idx];
                    LogRawDebug(LOG_SOURCE_IBUS, "%02X ", pkt[idx]);
                }
                if (memcmp(ibus->txBuffer[ibus->txBufferReadbackIdx], pkt, msgLength) == 0) {
                    LogRawDebug(LOG_SOURCE_IBUS, "[SELF]");
                    memset(ibus->txBuffer[ibus->txBufferReadbackIdx], 0, msgLength);
                    if (ibus->txBufferReadbackIdx + 1 == IBUS_TX_BUFFER_SIZE) {
                        ibus->txBufferReadbackIdx = 0;
                    } else {
                        ibus->txBufferReadbackIdx++;
                    }
                }
                LogRawDebug(LOG_SOURCE_IBUS, "\r\n");
                if (IBusValidateChecksum(pkt) == 1) {
                    uint8_t srcSystem = pkt[IBUS_PKT_SRC];
                    if (srcSystem == IBUS_DEVICE_BLUEBUS &&
                        pkt[IBUS_PKT_DST] == IBUS_DEVICE_LOC
                    ) {
                        IBusHandleBlueBusMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_RAD) {
                        IBusHandleRADMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_BMBT) {
                        IBusHandleBMBTMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_IKE) {
                        IBusHandleIKEMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_GT) {
                        IBusHandleGTMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_LCM) {
                        IBusHandleLCMMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_MID) {
                        IBusHandleMIDMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_NAVE) {
                        IBusHandleNAVMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_MFL) {
                        IBusHandleMFLMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_DSP) {
                        IBusHandleDSPMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_GM) {
                        IBusHandleGMMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_EWS) {
                        IBusHandleEWSMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_VM) {
                        IBusHandleVMMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_PDC) {
                        IBusHandlePDCMessage(ibus, pkt);
                    }
                    if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_TEL) {
                        IBusHandleTELMessage(ibus, pkt);
                    }
                } else {
                    LogError(
                        "IBus: %02X -> %02X Length: %d - Invalid Checksum",
                        pkt[IBUS_PKT_SRC],
                        pkt[IBUS_PKT_DST],
                        msgLength,
                        pkt[IBUS_PKT_LEN]
                    );
                }
                memset(ibus->rxBuffer, 0, IBUS_RX_BUFFER_SIZE);
                ibus->rxBufferIdx = 0;
            }
        }
        if (ibus->rxLastStamp == 0) {
            EventTriggerCallback(IBUS_EVENT_FIRST_MESSAGE_RECEIVED, 0);
        }
        ibus->rxLastStamp = TimerGetMillis();
    } else if (ibus->txBufferWriteIdx != ibus->txBufferReadIdx) {
        // Flush the transmit buffer out to the bus
        uint8_t txTimeout = IBUS_TX_TIMEOUT_OFF;
        uint8_t beginTxTimestamp = TimerGetMillis();
        while (
            ibus->txBufferWriteIdx != ibus->txBufferReadIdx &&
            txTimeout != IBUS_TX_TIMEOUT_ON
        ) {
            uint32_t now = TimerGetMillis();
            if ((now - ibus->txLastStamp) >= IBUS_TX_BUFFER_WAIT) {
                uint8_t msgLen = ibus->txBuffer[ibus->txBufferReadIdx][1] + 2;
                uint8_t idx;
                /*
                 * Make sure that the STATUS pin on the TH3122 is low, indicating no
                 * bus activity before transmitting
                 */
                if (IBUS_UART_STATUS == 0) {
                    for (idx = 0; idx < msgLen; idx++) {
                        ibus->uart.registers->uxtxreg = ibus->txBuffer[ibus->txBufferReadIdx][idx];
                        // Wait for the data to leave the TX buffer
                        while ((ibus->uart.registers->uxsta & (1 << 9)) != 0);
                    }
                    txTimeout = IBUS_TX_TIMEOUT_DATA_SENT;
                    if (ibus->txBufferReadIdx + 1 == IBUS_TX_BUFFER_SIZE) {
                        ibus->txBufferReadIdx = 0;
                    } else {
                        ibus->txBufferReadIdx++;
                    }
                    ibus->txLastStamp = TimerGetMillis();
                } else if (txTimeout != IBUS_TX_TIMEOUT_DATA_SENT) {
                    if ((now - beginTxTimestamp) > IBUS_TX_TIMEOUT_WAIT) {
                        txTimeout = IBUS_TX_TIMEOUT_ON;
                    }
                }
            }
        }
    }

    // Clear the RX Buffer if it's over the timeout or about to overflow
    if (ibus->rxBufferIdx > 0) {
        uint32_t now = TimerGetMillis();
        if ((now - ibus->rxLastStamp) > IBUS_RX_BUFFER_TIMEOUT ||
            (ibus->rxBufferIdx + 1) == IBUS_RX_BUFFER_SIZE
        ) {
            long long unsigned int ts = (long long unsigned int) TimerGetMillis();
            LogRawDebug(
                LOG_SOURCE_IBUS,
                "[%llu] ERROR: IBus: RX Buffer Timeout [%d]: ",
                ts,
                ibus->rxBufferIdx
            );
            uint8_t idx;
            for (idx = 0; idx < ibus->rxBufferIdx; idx++) {
                LogRawDebug(LOG_SOURCE_IBUS, "%02X ", ibus->rxBuffer[idx]);
            }
            LogRawDebug(LOG_SOURCE_IBUS, "\r\n");
            ibus->rxBufferIdx = 0;
            memset(ibus->rxBuffer, 0, IBUS_RX_BUFFER_SIZE);
        }
    }
    UARTReportErrors(&ibus->uart);
}

/**
 * IBusSendCommandInternal()
 *     Description:
 *         This function exists to implement message priority without changing
 *         every existing call to IBusSendCommand()
 *
 *         Priority high shoves the message into the front of the ring buffer
 *         Priortiy normal queues it to the end of the buffer
 *     Params:
 *         IBus_t *ibus
 *         const uint8_t src
 *         const uint8_t dst
 *         const uint8_t *data
 *     Returns:
 *         void
 */
static void IBusSendCommandInternal(
    IBus_t *ibus,
    const uint8_t src,
    const uint8_t dst,
    const uint8_t *data,
    const size_t dataSize,
    const uint8_t priority
) {
    if (dataSize + 4 >= IBUS_MAX_MSG_LENGTH) {
        LogWarning("IBus: Refuse to transmit frame of length %d", dataSize + 4);
        return;
    }
    // Calculate number of used slots in the ring buffer
    uint8_t usedSlots;
    if (ibus->txBufferWriteIdx >= ibus->txBufferReadIdx) {
        usedSlots = ibus->txBufferWriteIdx - ibus->txBufferReadIdx;
    } else {
        usedSlots = IBUS_TX_BUFFER_SIZE - ibus->txBufferReadIdx + ibus->txBufferWriteIdx;
    }
    // Check if buffer is full (one slot must remain empty to distinguish full from empty)
    if (usedSlots >= IBUS_TX_BUFFER_SIZE - 1) {
        long long unsigned int ts = (long long unsigned int) TimerGetMillis();
        LogRawDebug(
            LOG_SOURCE_IBUS,
            "[%llu] ERROR: IBus: TX Buffer Overflow.\r\n",
            ts
        );
        return;
    }
    uint8_t idx, msgSize;
    msgSize = dataSize + 4;
    uint8_t msg[msgSize];
    msg[0] = src;
    msg[1] = dataSize + 2;
    msg[2] = dst;
    // Add the Data to the packet
    memcpy(msg + 3, data, dataSize);
    // Calculate the CRC
    uint8_t crc = 0;
    uint8_t maxIdx = msgSize - 1;
    for (idx = 0; idx < maxIdx; idx++) {
        crc ^= msg[idx];
    }
    msg[msgSize - 1] = crc;
    uint8_t bufferIdx = 0;
    if (priority == IBUS_MSG_PRIORITY_NORMAL) {
        bufferIdx = ibus->txBufferWriteIdx;
        if (ibus->txBufferWriteIdx + 1 == IBUS_TX_BUFFER_SIZE) {
            ibus->txBufferWriteIdx = 0;
        } else {
            ibus->txBufferWriteIdx++;
        }
    } else {
        // Priority message, so queue it first
        // Calculate new read index (decrement with wrap-around)
        if (ibus->txBufferReadIdx == 0) {
            bufferIdx = IBUS_TX_BUFFER_SIZE - 1;
        } else {
            bufferIdx = ibus->txBufferReadIdx - 1;
        }
        ibus->txBufferReadIdx = bufferIdx;
        ibus->txBufferReadbackIdx = bufferIdx;
    }
    memcpy(ibus->txBuffer[bufferIdx], msg, msgSize);
}

/**
 * IBusSendCommand()
 *     Description:
 *         Take a Destination, source and message and add it to the transmit
 *         char queue so we can send it later.
 *     Params:
 *         IBus_t *ibus
 *         const uint8_t src
 *         const uint8_t dst
 *         const uint8_t *data
 *     Returns:
 *         void
 */
void IBusSendCommand(
    IBus_t *ibus,
    const uint8_t src,
    const uint8_t dst,
    const uint8_t *data,
    const size_t dataSize
) {
    IBusSendCommandInternal(ibus, src, dst, data, dataSize, IBUS_MSG_PRIORITY_NORMAL);
}


/***
 * IBusSetInternalIgnitionStatus()
 *     Description:
 *        Allow outside callers to set the current ignition state from the Bus
 *     Params:
 *         IBus_t *ibus
 *         uint8_t ignitionStatus - The ignition status
 *     Returns:
 *         void
 */
void IBusSetInternalIgnitionStatus(IBus_t *ibus, uint8_t ignitionStatus)
{
    if (ignitionStatus != IBUS_IGNITION_KL15) {
        ibus->ignitionStatus = ignitionStatus;
    }
    EventTriggerCallback(
        IBUS_EVENT_IKE_IGNITION_STATUS,
        &ignitionStatus
    );
    ibus->ignitionStatus = ignitionStatus;
}

/***
 * IBusGetDateTimeAsEpoch()
 *     Description:
 *        Convert an IBusDateTime_t to an epoch
 *     Params:
 *         IBusDateTime_t *dt
 *     Returns:
 *         uint32 - The 32-bit unsigned number of seconds since 1970
 */
uint32_t IBusGetDateTimeAsEpoch(IBusDateTime_t *dt)
{
    uint32_t days = 0;

    // Add days for complete years since 1970
    for (uint16_t y = 1970; y < dt->year; y++) {
        days += 365;
        if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
            days++;
        }
    }

    // Add days for complete months in current year
    for (uint8_t m = 1; m < dt->month; m++) {
        days += IBUS_DAYS_IN_MONTH[m - 1];
        if (
            m == 2 &&
            (
                (dt->year % 4 == 0 && dt->year % 100 != 0) ||
                dt->year % 400 == 0
            )
        ) {
            days++;
        }
    }
    // Add days in current month
    days += dt->day - 1;
    return (
        days * 86400UL + dt->hour * 3600UL + dt->min * 60UL + dt->sec
    );
}

/**
* IBusGetEpochAsDateTime()
*     Description:
*        Convert an epoch to an IBusDateTime_t
*     Params:
*         uint32_t epoch - The 32-bit unsigned number of seconds since 1970
*     Returns:
*         IBusDateTime_t - The date time structure
*/
IBusDateTime_t IBusGetEpochAsDateTime(uint32_t epoch)
{
    IBusDateTime_t dt;
    dt.sec = epoch % 60;
    epoch /= 60;
    dt.min = epoch % 60;
    epoch /= 60;
    dt.hour = epoch % 24;
    uint32_t days = epoch / 24;

    // Determine year
    dt.year = 1970;
    while (1) {
        uint16_t daysInYear = 365;
        if ((dt.year % 4 == 0 && dt.year % 100 != 0) || dt.year % 400 == 0) {
            daysInYear = 366;
        }
        if (days < daysInYear) {
            break;
        }
        days -= daysInYear;
        dt.year++;
    }

    // Determine month
    uint8_t isLeapYear = (dt.year % 4 == 0 && dt.year % 100 != 0) || dt.year % 400 == 0;
    dt.month = 1;
    for (uint8_t m = 0; m < 12; m++) {
        uint8_t daysThisMonth = IBUS_DAYS_IN_MONTH[m];
        if (m == 1 && isLeapYear) {
            daysThisMonth++;
        }
        if (days < daysThisMonth) {
            break;
        }
        days -= daysThisMonth;
        dt.month++;
    }
    dt.day = days + 1;

    return dt;
}

/***
 * IBusGetLMCodingIndex()
 *     Description:
 *        Get the light module coding index
 *     Params:
 *         uint8_t *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - the light module coding index
 */
uint8_t IBusGetLMCodingIndex(uint8_t *packet)
{
    uint8_t codingIndex = {
        packet[IBUS_LM_CI_ID_OFFSET]
    };
    return codingIndex;
}

/***
 * IBusGetLMDiagnosticIndex()
 *     Description:
 *        Get the light module diagnostic index
 *     Params:
 *         uint8_t *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - the light module diagnostic index
 */
uint8_t IBusGetLMDiagnosticIndex(uint8_t *packet)
{
    uint8_t diagnosticIndex = {
        packet[IBUS_LM_DI_ID_OFFSET]
    };
    return diagnosticIndex;
}

/***
 * IBusGetLMDimmerChecksum()
 *     Description:
 *        Get the dimmer checksum
 *     Params:
 *         uint8_t *packet - The Light Module Dimmer Status Packet
 *     Returns:
 *         uint8_t - the light module coding index
 */
uint8_t IBusGetLMDimmerChecksum(uint8_t *packet)
{
    // Do not use the XOR
    uint8_t frameLength = packet[IBUS_PKT_LEN] - 3;
    uint8_t index = 4;
    uint8_t checksum = 0x00;
    while (frameLength > 0) {
        checksum ^= packet[index];
        index++;
        frameLength--;
    }
    return checksum;
}

/**
 * IBusGetLMVariant()
 *     Description:
 *        Get the light module variant, as per EDIABAS:
 *        Group file: D_00D0.GRP
*         Version:    1.5.1
 *     Params:
 *         uint8_t *packet - Diagnostics ident packet
 *     Returns:
 *         uint8_t - The light module variant
 */
uint8_t IBusGetLMVariant(uint8_t *packet)
{
    uint8_t diagnosticIndex = IBusGetLMDiagnosticIndex(packet);
    uint8_t codingIndex = IBusGetLMCodingIndex(packet);
    uint8_t lmVariant = 0;

    LogRaw("\r\nIBus: LM DI: %02X CI: %02X\r\n", diagnosticIndex, codingIndex);

    if (diagnosticIndex < 0x10) {
        lmVariant = IBUS_LM_LME38;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LME38");
    } else if (diagnosticIndex == 0x10) {
        lmVariant = IBUS_LM_LCM;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LCM");
    } else if (diagnosticIndex == 0x11) {
        lmVariant = IBUS_LM_LCM_A;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LCM_A");
    } else if (diagnosticIndex == 0x12 && codingIndex == 0x16) {
        lmVariant = IBUS_LM_LCM_II;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LCM_II");
    } else if ((diagnosticIndex == 0x12 && codingIndex == 0x17) ||
               diagnosticIndex == 0x13
    ) {
        lmVariant = IBUS_LM_LCM_III;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LCM_III");
    } else if (diagnosticIndex == 0x14) {
        lmVariant = IBUS_LM_LCM_IV;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LCM_IV");
    } else if (diagnosticIndex >= 0x20 && diagnosticIndex <= 0x2f) {
        lmVariant = IBUS_LM_LSZ;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LSZ");
    } else if (diagnosticIndex == 0x30) {
        lmVariant = IBUS_LM_LSZ_2;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LSZ_2");
    }

    return lmVariant;
}

/**
 * IBusGetNavDiagnosticIndex()
 *     Description:
 *        Get the nav diagnostic index
 *     Params:
 *         uint8_t *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav hardware version
 */
uint8_t IBusGetNavDiagnosticIndex(uint8_t *packet)
{
    char diVersion[3] = {
        packet[IBUS_GT_DI_ID_OFFSET],
        packet[IBUS_GT_DI_ID_OFFSET + 1],
        '\0'
    };
    return UtilsStrToInt(diVersion);
}

/**
 * IBusGetNavHWVersion()
 *     Description:
 *        Get the nav type hardware version
 *     Params:
 *         uint8_t *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav hardware version
 */
uint8_t IBusGetNavHWVersion(uint8_t *packet)
{
    char hwVersion[3] = {
        packet[IBUS_GT_HW_ID_OFFSET],
        packet[IBUS_GT_HW_ID_OFFSET + 1],
        '\0'
    };
    return UtilsStrToInt(hwVersion);
}

/**
 * IBusGetNavSWVersion()
 *     Description:
 *        Get the nav type software version
 *     Params:
 *         uint8_t *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav software version
 */
uint8_t IBusGetNavSWVersion(uint8_t *packet)
{
    char swVersion[3] = {
        (char) packet[IBUS_GT_SW_ID_OFFSET],
        (char) packet[IBUS_GT_SW_ID_OFFSET + 1],
        '\0'
    };
    return UtilsStrToInt(swVersion);
}

/**
 * IBusGetNavType()
 *     Description:
 *        Get the nav type based on the hardware and software versions
 *     Params:
 *         uint8_t *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav type
 */
uint8_t IBusGetNavType(uint8_t *packet)
{
    uint8_t diagnosticIndex = IBusGetNavDiagnosticIndex(packet);
    uint8_t navType = 0;
    switch (diagnosticIndex) {
        case 1:
            navType = IBUS_GT_MKI;
            break;
        case 2:
        case 3:
            navType = IBUS_GT_MKII;
            break;
        case 4:
            navType = IBUS_GT_MKIII;
            break;
        case 5:
        case 6:
            navType = IBUS_GT_MKIV;
            break;
        default:
            navType = IBUS_GT_DETECT_ERROR;
            break;
    }
    uint8_t softwareVersion = IBusGetNavSWVersion(packet);
    if (navType == IBUS_GT_MKIII && softwareVersion >= 60) {
        navType = IBUS_GT_MKIII_NEW_UI;
    }
    if (navType == IBUS_GT_MKIV &&
        (softwareVersion <= 2 || softwareVersion >= 40)
    ) {
        navType = IBUS_GT_MKIV_STATIC;
    }
    return navType;
}

/**
 * IBusGetVehicleType()
 *     Description:
 *        Get the vehicle type from the cluster type response
 *     Params:
 *         uint8_t *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav type
 */
uint8_t IBusGetVehicleType(uint8_t *packet)
{
    uint8_t vehicleType = (packet[IBUS_PKT_DB1] >> 4) & 0xF;
    uint8_t detectedVehicleType = 0xFF;
    if (vehicleType == 0x04 || vehicleType == 0x06 || vehicleType == 0x0F) {
        detectedVehicleType = IBUS_VEHICLE_TYPE_E46;
    } else if (vehicleType == 0x0B) {
        detectedVehicleType = IBUS_VEHICLE_TYPE_R50;
    } else if (vehicleType == 0x0A) {
        detectedVehicleType = IBUS_VEHICLE_TYPE_E8X;
    } else {
        // 0x00 and 0x02 are possibilities here
        detectedVehicleType = IBUS_VEHICLE_TYPE_E38_E39_E52_E53;
    }
    return detectedVehicleType;
}

/**
 * IBusGetConfigTemp()
 *     Description:
 *        Get the configured temperature unit from cluster type response
 *     Params:
 *         uint8_t *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - the Celsius or Fahrhenheit configuration
 */
uint8_t IBusGetConfigTemp(uint8_t *packet)
{
    uint8_t tempUnit = (packet[IBUS_PKT_DB2] >> 1) & 0x1;
    return tempUnit;
}

/**
 * IBusGetConfigDistance()
 *     Description:
 *        Get the configured temperature unit from cluster type response
 *     Params:
 *         uint8_t *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - the KM or MILES configuration
 */
uint8_t IBusGetConfigDistance(uint8_t *packet)
{
    unsigned char distUnit = (packet[IBUS_PKT_DB2] >> 6) & 0x1;
    return distUnit;
}

/**
 * IBusGetConfigLanguage()
 *     Description:
 *        Get the configured Language
 *     Params:
 *         uint8_t *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - the language
 */
uint8_t IBusGetConfigLanguage(uint8_t *packet)
{
    uint8_t lang = packet[IBUS_PKT_DB1] & 0x0F;
    return lang;
}

/**
 * IBusCommandBlueBusSetStatus()
 *     Description:
 *        Sends a sub-status command and value to the local broadcast address
 *        so that the BlueBus can perform an action based on it
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t subCommand - The sub-command to emit
 *         uint8_t value - The value of the sub-command
 *     Returns:
 *         void
 */
void IBusCommandBlueBusSetStatus(IBus_t *ibus, uint8_t subCommand, uint8_t value)
{
    uint8_t statusMessage[] = {
        IBUS_BLUEBUS_CMD_SET_STATUS,
        subCommand,
        value
    };
    IBusSendCommand(ibus, IBUS_DEVICE_BLUEBUS, IBUS_DEVICE_LOC, statusMessage, 3);
}

/**
 * IBusCommandCDCAnnounce()
 *     Description:
 *        Send the CDC Announcement Message so the radio knows we're here
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandCDCAnnounce(IBus_t *ibus)
{
    const uint8_t cdcAlive[] = {0x02, 0x01};
    IBusSendCommand(ibus, IBUS_DEVICE_CDC, IBUS_DEVICE_LOC, cdcAlive, sizeof(cdcAlive));
}

/**
 * IBusCommandCDCPollResponse()
 *     Description:
 *        Respond to the Radio's "Ping" with our "Pong"
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandCDCPollResponse(IBus_t *ibus)
{
    const uint8_t cdcPing[] = {0x02, 0x00};
    // The RAD may AWOL the CDC if it sees another message addressed to it
    // arrive prior to the response from the CDC, so we need to send it ahead
    // of all other messages in the queue
    IBusSendCommandInternal(
        ibus,
        IBUS_DEVICE_CDC,
        IBUS_DEVICE_RAD,
        cdcPing,
        sizeof(cdcPing),
        IBUS_MSG_PRIORITY_HIGH
    );
}

/**
 * IBusCommandCDCStatus()
 *     Description:
 *        Respond to the Radio's status request
 *        Sample Packet from a factory iPod module:
 *          18 0E 68 39 00 82 00 60 00 07 11 00 01 00 0B CK
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t status - The current CDC status
 *         uint8_t function - The current CDC function
 *         uint8_t discCount - The number of discs to report loaded
 *         uint8_t discNumber - The disc number to report
 *     Returns:
 *         void
 */
void IBusCommandCDCStatus(
    IBus_t *ibus,
    uint8_t status,
    uint8_t function,
    uint8_t discCount,
    uint8_t discNumber
) {
    function = function + 0x80;
    const uint8_t cdcStatus[] = {
        IBUS_COMMAND_CDC_RESPONSE,
        status,
        function,
        0x00, // Errors
        discCount,
        0x00,
        discNumber,
        0x01, // Song Number
        0x00,
        0x01, // MP3 / Audio Text bit
        0x01, // Folder Number
        0x01  // File Number
    };
    // The RAD may AWOL the CDC if it sees another message addressed to it
    // arrive prior to the response from the CDC, so we need to send it ahead
    // of all other messages in the queue
    IBusSendCommandInternal(
        ibus,
        IBUS_DEVICE_CDC,
        IBUS_DEVICE_RAD,
        cdcStatus,
        sizeof(cdcStatus),
        IBUS_MSG_PRIORITY_HIGH
    );
}

/**
 * IBusCommandDIAGetCodingData()
 *     Description:
 *        Request the given systems coding data
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandDIAGetCodingData(
    IBus_t *ibus,
    uint8_t system,
    uint8_t addr,
    uint8_t offset
) {
    uint8_t msg[] = {0x08, 0x00, addr, offset};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, system, msg, 1);
}

/**
 * IBusCommandDIAGetIdentity()
 *     Description:
 *        Request the given systems identity info
 *        Raw: 3F LEN DST 00 CHK
 *        3F 00 D0 00 00
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandDIAGetIdentity(IBus_t *ibus, uint8_t system)
{
    uint8_t msg[] = {0x00};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, system, msg, 1);
}

/**
 * IBusCommandDIAGetIdentityPage()
 *     Description:
 *        Request the given systems identity info
 *        Raw: 3F LEN DST 00 PG CHK
 *        3F 00 3B 11 00
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t system - The system to target
 *         uint8_t page - The Identity Page to request
 *     Returns:
 *         void
 */
void IBusCommandDIAGetIdentityPage(IBus_t *ibus, uint8_t system, uint8_t page)
{
    uint8_t msg[] = {0x00, page};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, system, msg, 2);
}

/**
 * IBusCommandDIAGetIOStatus()
 *     Description:
 *        Request the IO Status of the given system
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandDIAGetIOStatus(IBus_t *ibus, uint8_t system)
{
    uint8_t msg[] = {0x0B};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_DIA,
        system,
        msg,
        1
    );
}

/**
 * IBusCommandDIAGetOSIdentity()
 *     Description:
 *        Request the OS identity
 *        Raw: 3F LEN DST 11 CHK
 *        3F 00 3B 11 00
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandDIAGetOSIdentity(IBus_t *ibus, uint8_t system)
{
    uint8_t msg[] = {0x11};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, system, msg, 1);
}

/**
 * IBusCommandDIATerminateDiag()
 *     Description:
 *        Terminate any ongoing diagnostic request on the given system
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandDIATerminateDiag(IBus_t *ibus, uint8_t system)
{
    uint8_t msg[] = {0x9F};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_DIA,
        system,
        msg,
        1
    );
}

/**
 * IBusCommandDSPSetMode()
 *     Description:
 *        Set the DSP mode
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t mode - The mode to set the DSP to
 *     Returns:
 *         void
 */
void IBusCommandDSPSetMode(IBus_t *ibus, uint8_t mode)
{
    uint8_t msg[] = {IBUS_DSP_CMD_CONFIG_SET, mode};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_DSP,
        msg,
        2
    );
}

/**
 * IBusCommandGetModuleStatus()
 *     Description:
 *        Request a "pong" from a given module to see if it is present
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t source - The system to send the request from
 *         uint8_t system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandGetModuleStatus(
    IBus_t *ibus,
    uint8_t source,
    uint8_t system
) {
    uint8_t msg[] = {0x01};
    IBusSendCommand(ibus, source, system, msg, 1);
}

/**
 * IBusCommandSetModuleStatus()
 *     Description:
 *        Request a "pong" from a given module to see if it is present
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t source - The system to send the request from
 *         uint8_t system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandSetModuleStatus(
    IBus_t *ibus,
    uint8_t source,
    uint8_t system,
    uint8_t status
) {
    uint8_t msg[] = {IBUS_CMD_MOD_STATUS_RESP, status};
    IBusSendCommand(ibus, source, system, msg, 2);
}

/**
 * IBusCommandGMDoorCenterLockButton()
 *     Description:
 *        Issue a diagnostic message to the GM to act as if the center
 *        lock button was pressed
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorCenterLockButton(IBus_t *ibus)
{
    if (
        ibus->vehicleType == IBUS_VEHICLE_TYPE_E46 ||
        ibus->vehicleType == IBUS_VEHICLE_TYPE_E8X
    ) {
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_CENTRAL_LOCK, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else {
        uint8_t gmVariant = ConfigGetSetting(CONFIG_GM_VARIANT_ADDRESS);
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM1_JOB_CENTRAL_LOCK, // Job
            0x01 // On / Off
        };
        switch (gmVariant) {
            case IBUS_GM_ZKE3_GM1:
            case IBUS_GM_ZKE3_GM4:
                msg[2] = IBUS_CMD_ZKE3_GM1_JOB_CENTRAL_LOCK;
                break;
            case IBUS_GM_ZKE3_GM5:
            case IBUS_GM_ZKE3_GM6:
                msg[2] =  IBUS_CMD_ZKE3_GM5_JOB_CENTRAL_LOCK;
                break;
        }
        if (msg[2] != 0x00) {
            IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
        }
    }
}

/**
 * IBusCommandGMDoorUnlockHigh()
 *     Description:
 *        Issue a diagnostic message to the GM to unlock the high side doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorUnlockHigh(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46 ||
        ibus->vehicleType == IBUS_VEHICLE_TYPE_E8X
    ) {
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_UNLOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E52_E53) {
        uint8_t msg[4] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM5_JOB_UNLOCK_HIGH, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorUnlockLow()
 *     Description:
 *        Issue a diagnostic message to the GM to unlock the low side doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorUnlockLow(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46 ||
        ibus->vehicleType == IBUS_VEHICLE_TYPE_E8X
    ) {
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_UNLOCK_LOW, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E52_E53) {
        uint8_t msg[4] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM5_JOB_UNLOCK_LOW, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorLockHigh()
 *     Description:
 *        Issue a diagnostic message to the GM to lock the high side doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorLockHigh(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46 ||
        ibus->vehicleType == IBUS_VEHICLE_TYPE_E8X
    ) {
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_LOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E52_E53) {
        uint8_t msg[4] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM5_JOB_LOCK_HIGH, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorLockLow()
 *     Description:
 *        Issue a diagnostic message to the GM to lock the low side doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorLockLow(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46 ||
        ibus->vehicleType == IBUS_VEHICLE_TYPE_E8X
    ) {
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_LOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E52_E53) {
        uint8_t msg[4] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM5_JOB_LOCK_LOW, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorUnlockAll()
 *     Description:
 *        Issue a diagnostic message to the GM to unlock all doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorUnlockAll(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46 ||
        ibus->vehicleType == IBUS_VEHICLE_TYPE_E8X
    ) {
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_UNLOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else {
        uint8_t gmVariant = ConfigGetSetting(CONFIG_GM_VARIANT_ADDRESS);
        uint8_t msg[4] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM1_JOB_CENTRAL_LOCK, // Job
            0x01 // On / Off
        };
        if (gmVariant >= IBUS_GM_ZKE3_GM1 && gmVariant <= IBUS_GM_ZKE3_GM4) {
            msg[2] = IBUS_CMD_ZKE3_GM1_JOB_CENTRAL_LOCK;
        } else if (gmVariant >= IBUS_GM_ZKE3_GM5) {
            msg[2] = IBUS_CMD_ZKE3_GM5_JOB_CENTRAL_LOCK;
        }
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorLockAll()
 *     Description:
 *        Issue a diagnostic message to the GM to lock all doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorLockAll(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46 ||
        ibus->vehicleType == IBUS_VEHICLE_TYPE_E8X
    ) {
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_LOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else {
        uint8_t gmVariant = ConfigGetSetting(CONFIG_GM_VARIANT_ADDRESS);
        uint8_t msg[4] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM1_JOB_LOCK_ALL, // Job
            0x01 // On / Off
        };
        if (gmVariant >= IBUS_GM_ZKE3_GM1 && gmVariant <= IBUS_GM_ZKE3_GM4) {
            msg[2] = IBUS_CMD_ZKE3_GM1_JOB_LOCK_ALL;
        } else if (gmVariant >= IBUS_GM_ZKE3_GM5) {
            msg[2] = IBUS_CMD_ZKE3_GM5_JOB_LOCK_ALL;
        }
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGTBMBTControl()
 *     Description:
 *        Issue a diagnostic message to the GM to lock all doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t status - The status to set the monitor to
 *     Returns:
 *         void
 */
void IBusCommandGTBMBTControl(IBus_t *ibus, uint8_t status)
{
    uint8_t msg[2] = {
        IBUS_CMD_GT_MONITOR_CONTROL,
        status,
    };
    IBusSendCommand(ibus, IBUS_DEVICE_GT, IBUS_DEVICE_BMBT, msg, 2);
}

void IBusCommandGTUpdate(IBus_t *ibus, uint8_t updateType)
{
    uint8_t msg[4] = {
        IBUS_CMD_GT_WRITE_WITH_CURSOR,
        updateType,
        0x01,
        0x00
    };
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, msg, 4);
}

static void IBusInternalCommandGTWriteIndex(
    IBus_t *ibus,
    uint8_t index,
    char *message,
    uint8_t indexMode
) {
    // @TODO: This is 14 for the older UI. Come up with a better solution
    uint8_t maxLength = 23;
    uint8_t length = strlen(message);
    if (length > maxLength) {
        length = maxLength;
    }
    const size_t pktLenght = length + 4;
    uint8_t text[pktLenght];
    memset(text, 0x00, pktLenght);
    text[0] = IBUS_CMD_GT_WRITE_NO_CURSOR;
    text[1] = indexMode;
    text[2] = 0x00;
    text[3] = index;
    memcpy(text + 4, message, length);
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

static void IBusCommandGTWriteIndexStaticInternal(
    IBus_t *ibus,
    uint8_t index,
    char *message,
    uint8_t cursorPos
) {
    uint8_t length = strlen(message);
    const size_t pktLenght = length + 4;
    uint8_t text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_WITH_CURSOR;
    text[1] = IBUS_CMD_GT_WRITE_STATIC;
    text[2] = cursorPos;
    text[3] = index;
    memcpy(text + 4, message, length);
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

/**
 * IBusCommandGTWriteBusinessNavTitle()
 *     Description:
 *        Write the single line available to write to on the Business Nav system
 *        It supports a maximum of 11 characters
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandGTWriteBusinessNavTitle(IBus_t *ibus, char *message) {
    uint8_t length = strlen(message);
    if (length > IBUS_TCU_SINGLE_LINE_UI_MAX_LEN) {
        length = IBUS_TCU_SINGLE_LINE_UI_MAX_LEN;
    }
    const uint8_t packetLength = length + 3;
    uint8_t text[packetLength];
    memset(text, 0, packetLength);
    text[0] = IBUS_CMD_GT_WRITE_TITLE;
    text[1] = 0x40;
    text[2] = 0x30;
    memcpy(text + 3, message, length);
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, packetLength);
}

void IBusCommandGTWriteIndex(
    IBus_t *ibus,
    uint8_t index,
    char *message
) {
    IBusInternalCommandGTWriteIndex(
        ibus,
        index,
        message,
        IBUS_CMD_GT_WRITE_INDEX
    );
}

void IBusCommandGTWriteIndexTMC(
    IBus_t *ibus,
    uint8_t index,
    char *message
) {
    IBusInternalCommandGTWriteIndex(
        ibus,
        index,
        message,
        IBUS_CMD_GT_WRITE_INDEX_TMC
    );
}

/**
 * IBusCommandGTWriteIndexTitle()
 *     Description:
 *        Write the TMC title "Strip"
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The text
 *     Returns:
 *         void
 */
void IBusCommandGTWriteIndexTitle(IBus_t *ibus, char *message) {
    uint8_t length = strlen(message);
    if (length > 20) {
        length = 20;
    }
    const size_t pktLenght = length + 6;
    uint8_t text[pktLenght];
    memset(text, 0x20, pktLenght);
    text[0] = IBUS_CMD_GT_WRITE_WITH_CURSOR;
    text[1] = IBUS_CMD_GT_WRITE_ZONE;
    text[2] = 0x01; // Cursor at 0
    text[3] = 0x49; // Write menu title index
    memcpy(text + 4, message, length);
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

/**
 * IBusCommandGTWriteIndexTitleNGUI()
 *     Description:
 *        Write the TMC title "Strip"
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The text
 *     Returns:
 *         void
 */
void IBusCommandGTWriteIndexTitleNGUI(IBus_t *ibus, char *message) {
    uint8_t length = strlen(message);
    if (length > 24) {
        length = 24;
    }
    const size_t pktLenght = length + 6;
    uint8_t text[pktLenght];
    memset(text, 0x06, pktLenght);
    text[0] = IBUS_CMD_GT_WRITE_NO_CURSOR;
    text[1] = IBUS_CMD_GT_WRITE_INDEX_TMC;
    text[2] = 0x00; // Cursor at 0
    text[3] = 0x09; // Write menu title index
    memcpy(text + 4, message, length);
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

void IBusCommandGTWriteIndexStatic(IBus_t *ibus, uint8_t index, char *message)
{
    uint8_t length = strlen(message);
    if (length > 40) {
        length = 40;
    }
    uint8_t cursorPos = 0;
    uint8_t currentIdx = 0;
    while (currentIdx < length) {
        uint8_t textLength = length - currentIdx;
        if (textLength > 0x14) {
            textLength = 0x14;
        }
        char msg[textLength + 1];
        memset(msg, '\0', sizeof(msg));
        memcpy(msg, message + currentIdx, textLength);
        currentIdx += textLength;
        if (cursorPos == 0) {
            IBusCommandGTWriteIndexStaticInternal(ibus, index, msg, 1);
        } else {
            IBusCommandGTWriteIndexStaticInternal(ibus, index, msg, cursorPos);
        }
        // Make sure we do not write over the
        // last character of the previous string
        cursorPos = cursorPos + textLength + 1;
    }
}

/**
 * IBusCommandGTWriteTitleArea()
 *     Description:
 *        Write the title using the "old" UI "Area" update message
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The text
 *     Returns:
 *         void
 */
void IBusCommandGTWriteTitleArea(IBus_t *ibus, char *message)
{
    uint8_t length = strlen(message);
    if (length > 9) {
        length = 9;
    }
    // Length + Write Type + Write Area + Size
    const size_t pktLenght = length + 3;
    uint8_t text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_TITLE;
    text[1] = IBUS_CMD_GT_WRITE_ZONE;
    text[2] = 0x30;
    memcpy(text + 3, message, length);
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

/**
 * IBusCommandGTWriteTitleIndex()
 *     Description:
 *        Write the title using the "new" UI "Index" update message.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The text
 *     Returns:
 *         void
 */
void IBusCommandGTWriteTitleIndex(IBus_t *ibus, char *message)
{
    uint8_t length = strlen(message);
    if (length > 9) {
        length = 9;
    }
    // Length + Write Type + Write Area + Write Index + Size
    const size_t pktLenght = length + 4;
    uint8_t text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_NO_CURSOR;
    text[1] = IBUS_CMD_GT_WRITE_ZONE;
    text[2] = 0x01; // Unused in this layout
    text[3] = 0x40; // Write Area 0 Index
    memcpy(text + 4, message, length);
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

void IBusCommandGTWriteTitleC43(IBus_t *ibus, char *message)
{
    uint8_t length = strlen(message);
    if (length > 11) {
        length = 11;
    }
    // Length + Write Type + Write Area + Size + Watermark
    const size_t pktLenght = length + 8;
    uint8_t text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_TITLE;
    text[1] = 0x40;
    text[2] = 0x20;
    memcpy(text + 3, message, length);
    text[length + 3] = 0x04;
    text[length + 4] = 0x20;
    text[length + 5] = 0x20;
    text[length + 6] = 0x20;
    // "Watermark" Any update we send, so we know that it was us
    text[length + 7] = IBUS_RAD_MAIN_AREA_WATERMARK;
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

void IBusCommandGTWriteZone(IBus_t *ibus, uint8_t index, char *message)
{
    uint8_t length = strlen(message);
    const size_t pktLenght = length + 4;
    uint8_t text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_WITH_CURSOR;
    text[1] = IBUS_CMD_GT_WRITE_ZONE;
    text[2] = 0x01;
    text[3] = index;
    memcpy(text + 4, message, length);
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

/**
 * IBusCommandIKEGetIgnitionStatus()
 *     Description:
 *        Send the command to request the ignition status
 *        Raw: 18 03 80 10 8B
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandIKEGetIgnitionStatus(IBus_t *ibus)
{
    uint8_t msg[] = {IBUS_CMD_IKE_IGN_STATUS_REQ};
    IBusSendCommand(ibus, IBUS_DEVICE_CDC, IBUS_DEVICE_IKE, msg, 1);
}

/**
 * IBusCommandIKEGetVehicleConfig()
 *     Description:
 *        Request the vehicle configuration from the IKE
 *        Raw: 68 03 80 14 FF
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandIKEGetVehicleConfig(IBus_t *ibus)
{
    uint8_t msg[] = {IBUS_CMD_IKE_REQ_VEHICLE_TYPE};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_IKE,
        msg,
        1
    );
}

/**
 * IBusCommandIKEOBCControl()
 *     Description:
 *        Asks IKE for OBC control of the given property
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t property - The given property to control
 *         uint8_t control - The active to take on the given property
 *     Returns:
 *         void
 */
void IBusCommandIKEOBCControl(IBus_t *ibus, uint8_t property, uint8_t control)
{
    uint8_t controlMessage[] = {IBUS_CMD_OBC_CONTROL, property, control};
    IBusSendCommand(ibus, IBUS_DEVICE_GT, IBUS_DEVICE_IKE, controlMessage, 3);
}

/**
 * IBusCommandIKEIgnitionStatus()
 *     Description:
 *        Masquerade as the IKE sending the Ignition status message.
 *        This is useful only on a bench without an IKE present.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t status - The ignition status to set
 *     Returns:
 *         void
 */
void IBusCommandIKESetIgnitionStatus(IBus_t *ibus, uint8_t status)
{
    uint8_t statusMessage[2] = {IBUS_CMD_IKE_IGN_STATUS_RESP, status};
    IBusSendCommand(ibus, IBUS_DEVICE_IKE, IBUS_DEVICE_GLO, statusMessage, 2);
}

/**
 * IBusCommandIKESetTime()
 *     Description:
 *        Set the current time
 *        Raw: 3B 06 80 40 01 HH MM CS
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandIKESetTime(IBus_t *ibus, uint8_t hour, uint8_t minute)
{
    uint8_t msg[] = {
        IBUS_CMD_IKE_SET_REQUEST,
        IBUS_CMD_IKE_SET_REQUEST_TIME,
        hour,
        minute
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_GT,
        IBUS_DEVICE_IKE,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandIKESetDate()
 *     Description:
 *        Set the current time
 *        Raw: 3B 06 80 40 02 DD MM YY CS
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t year - The year
 *         uint8_t mon - The month
 *         uint8_t day - The day
 *     Returns:
 *         void
 */
void IBusCommandIKESetDate(IBus_t *ibus, uint8_t year, uint8_t mon, uint8_t day)
{
    uint8_t msg[] = {
        IBUS_CMD_IKE_SET_REQUEST,
        IBUS_CMD_IKE_SET_REQUEST_DATE,
        day,
        mon,
        year
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_GT,
        IBUS_DEVICE_IKE,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandTELIKEDisplayWrite()
 *     Description:
 *        Send text to the Business Radio
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The to display
 *     Returns:
 *         void
 */
void IBusCommandTELIKEDisplayWrite(IBus_t *ibus, char *message)
{
    uint8_t len = strlen(message);
    // Max display width
    if (len > 16) {
        len = 16;
    }
    uint8_t displayText[len + 3];
    memset(&displayText, 0, sizeof(displayText));
    displayText[0] = 0x23;
    displayText[1] = 0x42;
    displayText[2] = 0x32;
    if (len > 0) {
        memcpy(displayText + 3, message, len);
    }
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_TEL,
        IBUS_DEVICE_IKE,
        displayText,
        sizeof(displayText)
    );
}

/**
 * IBusCommandTELIKEDisplayClear()
 *     Description:
 *        Send an empty string to the Business Radio to clear the display
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandTELIKEDisplayClear(IBus_t *ibus)
{
    IBusCommandTELIKEDisplayWrite(ibus, "");
}

/**
 * IBusCommandIKECheckControlDisplayWrite()
 *     Description:
 *        Send a check control message to the High OBC display
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *text - The text to display
 *     Returns:
 *         void
 */
void IBusCommandIKECheckControlDisplayWrite(IBus_t *ibus, char *text)
{
    uint8_t textLen = strlen(text);
    // Check control display requires exactly 20 characters
    uint8_t paddedLen = 20;
    uint8_t msgLen = paddedLen + 3;
    uint8_t msg[msgLen];
    memset(&msg, 0, msgLen);
    msg[0] = IBUS_CMD_IKE_CCM_WRITE_TEXT;
    msg[1] = IBUS_DATA_IKE_CCM_WRITE_PERSIST_TEXT;
    msg[2] = 0x00;
    memset(msg + 3, 0x20, paddedLen);
    memcpy(msg + 3, text, (textLen < paddedLen) ? textLen : paddedLen);
    IBusSendCommand(ibus, IBUS_DEVICE_PDC, IBUS_DEVICE_IKE, msg, msgLen);
}

/**
 * IBusCommandIKECheckControlDisplayClear()
 *     Description:
 *        Send an empty string to the High OBC display to clear it
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandIKECheckControlDisplayClear(IBus_t *ibus)
{
    uint8_t msg[3] = {
        IBUS_CMD_IKE_CCM_WRITE_TEXT,
        IBUS_DATA_IKE_CCM_WRITE_CLEAR_TEXT,
        0x00
    };
    IBusSendCommand(ibus, IBUS_DEVICE_PDC, IBUS_DEVICE_IKE, msg, sizeof(msg));
}

/**
 * IBusCommandIKENumbericDisplayWrite()
 *     Description:
 *        Send a message to write the numeric display on the low OBC
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t number - The number to write to the screen
 *     Returns:
 *         void
 */
void IBusCommandIKENumbericDisplayWrite(IBus_t *ibus, uint8_t number)
{
    uint8_t msg[3] = {IBUS_CMD_IKE_WRITE_NUMERIC, IBUS_DATA_IKE_NUMERIC_WRITE, number};
    IBusSendCommand(ibus, IBUS_DEVICE_PDC, IBUS_DEVICE_IKE, msg, sizeof(msg));
}

/**
 * IBusCommandIKENumbericDisplayClear()
 *     Description:
 *     Send a message to clear the numeric display on the low OBC
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandIKENumbericDisplayClear(IBus_t *ibus)
{
    uint8_t msg[3] = {IBUS_CMD_IKE_WRITE_NUMERIC, IBUS_DATA_IKE_NUMERIC_CLEAR, 0x00};
    IBusSendCommand(ibus, IBUS_DEVICE_PDC, IBUS_DEVICE_IKE, msg, sizeof(msg));
}

/**
 * IBusCommandIRISDisplayWrite()
 *     Description:
 *        Write the IRIS display
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *text - The text to write
 *     Returns:
 *         void
 */
void IBusCommandIRISDisplayWrite(IBus_t *ibus, char *text)
{
    uint8_t len = strlen(text);
    uint8_t frameSize = len + 3;
    uint8_t displayText[frameSize];
    memset(&displayText, 0, frameSize);
    displayText[0] = IBUS_CMD_RAD_UPDATE_MAIN_AREA;
    displayText[1] = 0x00;
    displayText[2] = 0x30;
    memcpy(displayText + 3, text, len);
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_IRIS,
        displayText,
        frameSize
    );

}

/**
 * IBusCommandLMActivateBulbs()
 *     Description:
 *        Light module diagnostics: Activate bulbs
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t blinkerSide - left or right blinker
 *         uint8_t parkingLights - Activate the parking lights
 *     Returns:
 *         void
 */
void IBusCommandLMActivateBulbs(
    IBus_t *ibus,
    uint8_t blinkerSide,
    uint8_t parkingLights
) {
    uint8_t blinker = IBUS_LSZ_BLINKER_OFF;
    uint8_t parkingLightLeft = IBUS_LM_BULB_OFF;
    uint8_t parkingLightRight = IBUS_LM_BULB_OFF;
    if (ibus->lmVariant == IBUS_LM_LME38) {
        switch (blinkerSide) {
            case IBUS_LM_BLINKER_LEFT:
                blinker = IBUS_LME38_BLINKER_LEFT;
                break;
            case IBUS_LM_BLINKER_RIGHT:
                blinker = IBUS_LME38_BLINKER_RIGHT;
                break;
            case IBUS_LM_BLINKER_OFF:
                blinker = IBUS_LME38_BLINKER_OFF;
                break;
        }
        if (parkingLights == 0x01) {
            parkingLightLeft = IBUS_LME38_SIDE_MARKER_LEFT;
            parkingLightRight = IBUS_LME38_SIDE_MARKER_RIGHT;
        }
        // No LWR load sensor/HVAC pot voltage for LME38
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            blinker,
            0x00,
            parkingLightLeft,
            parkingLightRight,
            0x00,
            0x00,
            0x00,
            ibus->lmDimmerVoltage,
            0x00,
            0x00,
            0x00,
            0x00,
        };
        IBusSendCommand(
            ibus,
            IBUS_DEVICE_DIA,
            IBUS_DEVICE_LCM,
            msg,
            sizeof(msg)
        );
    } else if (
        ibus->lmVariant == IBUS_LM_LCM ||
        ibus->lmVariant == IBUS_LM_LCM_A
    ) {
        switch (blinkerSide) {
            case IBUS_LM_BLINKER_LEFT:
                blinker = IBUS_LCM_BLINKER_LEFT;
                break;
            case IBUS_LM_BLINKER_RIGHT:
                blinker = IBUS_LCM_BLINKER_RIGHT;
                break;
            case IBUS_LM_BLINKER_OFF:
                blinker = IBUS_LCM_BLINKER_OFF;
                break;
        }
        if (parkingLights == 0x01) {
            parkingLightLeft = IBUS_LCM_SIDE_MARKER_LEFT;
            parkingLightRight = IBUS_LCM_SIDE_MARKER_RIGHT;
        }
        // This is an issue! I'm not sure what differentiates S1 and S2.
        // It's meaningful enough a distinction that it is displayed in INPA.
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // S2_BLK_L, S2_BLK_R 0
            blinker, // S1_BLK_L, S1_BLK_R 1
            0x00,
            0x00,
            0x00,
            parkingLightLeft,
            parkingLightRight,
            0x00,
            0x00,
            ibus->lmDimmerVoltage,
            ibus->lmLoadRearVoltage,
            0x00
        };
        IBusSendCommand(
            ibus,
            IBUS_DEVICE_DIA,
            IBUS_DEVICE_LCM,
            msg,
            sizeof(msg)
        );
    } else if (
        ibus->lmVariant == IBUS_LM_LCM_II ||
        ibus->lmVariant == IBUS_LM_LCM_III ||
        ibus->lmVariant == IBUS_LM_LCM_IV
    ) {
        switch (blinkerSide) {
            case IBUS_LM_BLINKER_LEFT:
                blinker = IBUS_LCM_II_BLINKER_LEFT;
                break;
            case IBUS_LM_BLINKER_RIGHT:
                blinker = IBUS_LCM_II_BLINKER_RIGHT;
                break;
            case IBUS_LM_BLINKER_OFF:
                blinker = IBUS_LCM_II_BLINKER_OFF;
                break;
        }
        if (parkingLights == 0x01) {
            parkingLightLeft = IBUS_LCM_SIDE_MARKER_LEFT;
            parkingLightRight = IBUS_LCM_SIDE_MARKER_RIGHT;
        }
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00,
            0x00,
            blinker,
            0x00,
            0x00,
            parkingLightLeft,
            parkingLightRight,
            0x00,
            0x00,
            ibus->lmDimmerVoltage,
            ibus->lmLoadRearVoltage,
            0x00
        };
        IBusSendCommand(
            ibus,
            IBUS_DEVICE_DIA,
            IBUS_DEVICE_LCM,
            msg,
            sizeof(msg)
        );
    } else if (
        ibus->lmVariant == IBUS_LM_LSZ ||
        ibus->lmVariant == IBUS_LM_LSZ_2
    ) {
        switch (blinkerSide) {
          case IBUS_LM_BLINKER_LEFT:
                blinker = IBUS_LSZ_BLINKER_LEFT;
                break;
          case IBUS_LM_BLINKER_RIGHT:
                blinker = IBUS_LSZ_BLINKER_RIGHT;
                break;
          case IBUS_LM_BLINKER_OFF:
                blinker = IBUS_LSZ_BLINKER_OFF;
                break;
        }
        if (parkingLights == 0x01) {
            parkingLightLeft = IBUS_LSZ_SIDE_MARKER_LEFT;
            parkingLightRight = IBUS_LSZ_SIDE_MARKER_RIGHT;
        }
        uint8_t msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00,
            0x00,
            IBUS_LSZ_HEADLIGHT_OFF,
            blinker,
            parkingLightRight,
            parkingLightLeft,
            0x00,
            ibus->lmLoadFrontVoltage,
            0x00,
            ibus->lmDimmerVoltage,
            ibus->lmLoadRearVoltage,
            ibus->lmPhotoVoltage,
            0x00,
            0x00,
            0x00
        };
        IBusSendCommand(
            ibus,
            IBUS_DEVICE_DIA,
            IBUS_DEVICE_LCM,
            msg,
            sizeof(msg)
        );
    }
}


/**
 * IBusCommandLMGetClusterIndicators()
 *     Description:
 *        Query the Light Module for the cluster indicators
 *        Raw: 80 03 D0 53 00
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandLMGetClusterIndicators(IBus_t *ibus)
{
    uint8_t msg[] = {IBUS_LCM_LIGHT_STATUS_REQ};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_BMBT,
        IBUS_DEVICE_LCM,
        msg,
        sizeof(msg)
    );
}


/**
 * IBusCommandLMGetRedundantData()
 *     Description:
 *        Query the Light Module for the vehicle redundant data (VIN, Mileage)
 *        Raw: 80 03 D0 53 00
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandLMGetRedundantData(IBus_t *ibus)
{
    uint8_t msg[] = {IBUS_CMD_LCM_REQ_REDUNDANT_DATA};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_IKE,
        IBUS_DEVICE_LCM,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandMIDButtonPress()
 *     Description:
 *        Send a button press or release
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char dest - The to destination system
 *         char button - The button press to send
 *     Returns:
 *         void
 */
void IBusCommandMIDButtonPress(
    IBus_t *ibus,
    uint8_t dest,
    uint8_t button
) {
    uint8_t msg[] = {
        IBUS_MID_BUTTON_PRESS,
        0x00,
        0x00,
        button
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_MID,
        dest,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandMIDDisplayRADTitleText()
 *     Description:
 *        Send the main RAD area text to the MID screen.
 *        Add a watermark so we do not fight ourselves writing to the screen
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The to display on the MID
 *     Returns:
 *         void
 */
void IBusCommandMIDDisplayRADTitleText(IBus_t *ibus, char *message)
{
    uint8_t textLength = strlen(message);
    if (textLength > IBUS_MID_TITLE_MAX_CHARS) {
        textLength = IBUS_MID_TITLE_MAX_CHARS;
    }
    uint8_t displayText[textLength + 4];
    memset(&displayText, 0, textLength + 4);
    displayText[0] = IBUS_CMD_RAD_WRITE_MID_DISPLAY;
    displayText[1] = 0xC0;
    displayText[2] = 0x20;
    memcpy(displayText + 3, message, textLength);
    displayText[textLength + 3] = IBUS_RAD_MAIN_AREA_WATERMARK;
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_MID,
        displayText,
        sizeof(displayText)
    );
}

/**
 * IBusCommandMIDDisplayText()
 *     Description:
 *        Send text to the MID screen
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The to display on the MID
 *     Returns:
 *         void
 */
void IBusCommandMIDDisplayText(IBus_t *ibus, char *message)
{
    uint8_t len = strlen(message);
    if (len > IBUS_MID_MAX_CHARS) {
        len = IBUS_MID_MAX_CHARS;
    }
    uint8_t displayText[len + 3];
    memset(&displayText, 0, sizeof(displayText));
    displayText[0] = IBUS_CMD_RAD_WRITE_MID_DISPLAY;
    displayText[1] = 0x40;
    displayText[2] = 0x20;
    memcpy(displayText + 3, message, len);
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_TEL,
        IBUS_DEVICE_MID,
        displayText,
        sizeof(displayText)
    );
}

/**
 * IBusCommandMIDMenuWriteMany()
 *     Description:
 *        Send text to the MID menu
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t startIdx - The index to use
 *         char *menu - The menu to write to the MID
 *     Returns:
 *         void
 */
void IBusCommandMIDMenuWriteMany(
    IBus_t *ibus,
    uint8_t startIdx,
    uint8_t *menu,
    uint8_t menuLength
) {
    uint8_t menuText[menuLength + 4];
    menuText[0] = IBUS_CMD_RAD_WRITE_MID_MENU;
    menuText[1] = 0x40;
    menuText[2] = 0x00;
    menuText[3] = startIdx;
    memcpy(menuText + 4, menu, menuLength);
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_TEL,
        IBUS_DEVICE_MID,
        menuText,
        sizeof(menuText)
    );
}

/**
 * IBusCommandMIDMenuText()
 *     Description:
 *        Send text to the MID menu
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *text - The to display on the MID
 *     Returns:
 *         void
 */
void IBusCommandMIDMenuWriteSingle(
    IBus_t *ibus,
    uint8_t idx,
    char *text
) {
    uint8_t textLength = strlen(text);
    if (textLength > IBUS_MID_MENU_MAX_CHARS) {
        textLength = IBUS_MID_MENU_MAX_CHARS;
    }
    uint8_t menuText[textLength + 4];
    menuText[0] = IBUS_CMD_RAD_WRITE_MID_MENU;
    menuText[1] = 0xC3;
    menuText[2] = 0x00;
    menuText[3] = 0x40 + idx;
    memcpy(menuText + 4, text, textLength);
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_TEL,
        IBUS_DEVICE_MID,
        menuText,
        sizeof(menuText)
    );
}

/**
 * IBusCommandMIDSetMode()
 *     Description:
 *        Tell the MID that the given system wants to write to the screen
 *        Parameters Known:
 *            0x00 - Enable the main display and menus buttons
 *            0x02 - Disable the main display and menus buttons
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t system - The system to send the mode request from
 *         uint8_t param - The parameter to set
 *     Returns:
 *         void
 */
void IBusCommandMIDSetMode(
    IBus_t *ibus,
    uint8_t system,
    uint8_t param
) {
    uint8_t msg[] = {
        IBUS_MID_CMD_SET_MODE,
        param
    };
    IBusSendCommand(
        ibus,
        system,
        IBUS_DEVICE_MID,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandPDCGetSensorStatus()
 *     Description:
 *        Ask the PDC module for the distance reported by each sensor
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandPDCGetSensorStatus(IBus_t *ibus)
{
    uint8_t msg[] = {IBUS_CMD_PDC_SENSOR_REQUEST};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_PDC, msg, 1);
}

/**
 * IBusCommandRADC43ScreenModeSet()
 *     Description:
 *        Send the command that the C43 sends to update the screen mode
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t mode - The mode to broadcast
 *     Returns:
 *         void
 */
void IBusCommandRADC43ScreenModeSet(IBus_t *ibus, uint8_t mode)
{
    uint8_t msg[4] = {
        IBUS_CMD_RAD_C43_SCREEN_UPDATE,
        IBUS_CMD_RAD_C43_SET_MENU_MODE,
        0x00,
        mode
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_GT,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandRADCDCRequest()
 *     IBusCommandRADCDCRequest:
 *        Issue a command from the RAD to CDC
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t command - The command to send the CD Changer
 *     Returns:
 *         void
 */
void IBusCommandRADCDCRequest(IBus_t *ibus, uint8_t command)
{
    uint8_t msg[] = {IBUS_COMMAND_CDC_REQUEST, command, 0x00};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_CDC,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandRADClearMenu()
 *     Description:
 *        Clear the Radio Menu. The first bit here tells the GT to clear the
 *        screen. We're using 0x0B to attempt to keep certain radios from
 *        realizing that the screen has been cleared
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandRADClearMenu(IBus_t *ibus)
{
    uint8_t msg[] = {0x46, 0x0A};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_GT,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandRADDisableMenu()
 *     Description:
 *        Disable the Radio Menu
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandRADDisableMenu(IBus_t *ibus)
{
    uint8_t msg[] = {0x45, 0x02};
    // VMs handle Audio + OBC differently. Failing to handle this will crash the VM
    if (ibus->moduleStatus.NAV == 0) {
        msg[1] = 0x03;
    }
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_GT,
        IBUS_DEVICE_RAD,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandRADEnableMenu()
 *     Description:
 *        Enable the Radio Menu
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandRADEnableMenu(IBus_t *ibus)
{
    uint8_t msg[] = {0x45, 0x00};
    // VMs handle Audio + OBC differently. Failing to handle this will crash the VM
    if (ibus->moduleStatus.NAV == 0) {
        msg[1] = 0x01;
    }
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_GT,
        IBUS_DEVICE_RAD,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandRADExitMenu()
 *     Description:
 *        Exit the radio menu and return to the BMBT home screen
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandRADExitMenu(IBus_t *ibus)
{
    uint8_t msg[] = {0x45, 0x91};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_GT,
        IBUS_DEVICE_RAD,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandSESSetMapZoom()
 *     Description:
 *        Set the Navigation Zoom level via SES commands
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t zoomLevel - The requested zoom level
 *     Returns:
 *         void
 */
void IBusCommandSESSetMapZoom(IBus_t *ibus, uint8_t zoomLevel)
{
    uint8_t msg[] = {
        IBUS_SES_CMD_NAV_CTRL,
        IBUS_SES_DATA_NAV_CTRL_SETZOOM,
        IBUS_SES_NAV_ZOOM_CONSTANT[zoomLevel]
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_SES,
        IBUS_DEVICE_NAVE,
        msg,
        3
    );
}

/**
 * IBusCommandSESRouteFuel()
 *     Description:
 *        Find nearby Fuel Stations via SES command
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandSESRouteFuel(IBus_t *ibus)
{
    uint8_t msg[] = {
        IBUS_SES_CMD_NAV_CTRL,
        IBUS_SES_DATA_NAV_CTRL_ROUTEFUEL,
        0x03
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_SES,
        IBUS_DEVICE_NAVE,
        msg,
        3
    );
}

/**
 * IBusCommandSESSilentNavigation()
 *     Description:
 *        Disable Voice Guidance via SES command
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandSESSilentNavigation(IBus_t *ibus)
{
    uint8_t msg[] = {
        IBUS_SES_CMD_NAV_CTRL,
        IBUS_SES_DATA_NAV_CTRL_SILENCE,
        0x00
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_SES,
        IBUS_DEVICE_NAVE,
        msg,
        3
    );
}

/**
 * IBusCommandSESShowMap()
 *     Description:
 *        Display Map via SES command
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandSESShowMap(IBus_t *ibus)
{
    uint8_t msg[] = {
        IBUS_SES_CMD_NAV_CTRL,
        IBUS_SES_DATA_NAV_CTRL_SHOWMAP,
        0x00
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_SES,
        IBUS_DEVICE_NAVE,
        msg,
        3
    );
}

/**
 * IBusCommandSetVolume()
 *     Description:
 *        Exit the radio menu and return to the BMBT home screen
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t source - The source system of the command
 *         uint8_t dest - The destination system of the command
 *         uint8_t volume - The volume and direction to issue
 *     Returns:
 *         void
 */
void IBusCommandSetVolume(
    IBus_t *ibus,
    uint8_t source,
    uint8_t dest,
    uint8_t volume
) {
    uint8_t msg[] = {IBUS_CMD_VOLUME_SET, volume};
    IBusSendCommand(
        ibus,
        source,
        dest,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandTELBodyText()
 *     Description:
 *        Write body text with cursor offset to a telephone layout (0xA5 command).
 *        Used for SMS messages and list layouts. Allows writing long lines in chunks
 *        using the cursor offset to avoid GT buffer overflow.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t dest - Destination device
 *         uint8_t layout - Layout type (IBUS_TEL_LAYOUT_LIST or IBUS_TEL_LAYOUT_DETAIL)
 *         uint8_t offset - Cursor offset
 *         uint8_t options - Options bitfield (index | CLEAR | BUFFER | HIGHLIGHT)
 *         char *text - The text to display
 *     Returns:
 *         void
 */
void IBusCommandTELBodyText(
    IBus_t *ibus,
    uint8_t dest,
    uint8_t layout,
    uint8_t offset,
    uint8_t options,
    char *text
) {
    uint8_t textLength = strlen(text);
    uint8_t msgLength = textLength + 4;
    uint8_t msg[msgLength];
    msg[0] = IBUS_TEL_CMD_BODY_TEXT;
    msg[1] = layout;
    msg[2] = offset;
    msg[3] = options;
    if (textLength > 0) {
        memcpy(msg + 4, text, textLength);
    }
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, dest, msg, msgLength);
}

/**
 * IBusCommandTELCallTime()
 *     Description:
 *        Display call duration on the telephone info layout.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t dest - Destination device
 *         uint8_t minutes - Call duration minutes (0-999)
 *         uint8_t seconds - Call duration seconds (0-59)
 *     Returns:
 *         void
 */
void IBusCommandTELCallTime(IBus_t *ibus, uint8_t dest, uint8_t minutes, uint8_t seconds)
{
    char minStr[4] = {0};
    snprintf(minStr, sizeof(minStr), "%3d", minutes);
    uint8_t minMsg[6];
    minMsg[0] = IBUS_TEL_CMD_PROPERTY_TEXT;
    minMsg[1] = IBUS_TEL_PROP_CALL_TIME_MINUTES;
    minMsg[2] = 0x00;
    memcpy(minMsg + 3, minStr, 3);
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, dest, minMsg, 6);

    char secStr[3] = {0};
    snprintf(secStr, sizeof(secStr), "%02d", seconds % 60);
    uint8_t secMsg[5];
    secMsg[0] = IBUS_TEL_CMD_PROPERTY_TEXT;
    secMsg[1] = IBUS_TEL_PROP_CALL_TIME_SECONDS;
    secMsg[2] = 0x00;
    memcpy(secMsg + 3, secStr, 2);
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, dest, secMsg, 5);
}

/**
 * IBusCommandTELHandsfreeIndicator()
 *     Description:
 *        Show or hide the handsfree indicator character on displays.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t show - Show / hide indicator (bool)
 *     Returns:
 *         void
 */
void IBusCommandTELHandsfreeIndicator(IBus_t *ibus, uint8_t show)
{
    if (show) {
        uint8_t msg[] = {
            IBUS_TEL_CMD_TITLE_TEXT,
            IBUS_TEL_TITLE_ON_CALL_HFS,
            0x00,
            IBUS_TEL_CHAR_HANDSFREE_ICON
        };
        IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_ANZV, msg, sizeof(msg));
    } else {
        uint8_t msg[] = {
            IBUS_TEL_CMD_TITLE_TEXT,
            IBUS_TEL_TITLE_ON_CALL_HFS,
            0x00
        };
        IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_ANZV, msg, sizeof(msg));
    }
}

/**
 * IBusCommandTELLED()
 *     Description:
 *        Control the red/yellow/green LED indicators on the BMBT and MID (0x2B command).
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t leds - LED bitfield (combine IBUS_TEL_LED_* values)
 *     Returns:
 *         void
 */
void IBusCommandTELLED(IBus_t *ibus, uint8_t leds)
{
    uint8_t msg[] = {IBUS_TEL_CMD_LED_STATUS, leds};
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_ANZV, msg, sizeof(msg));
}

/**
 * IBusCommandTELMenuText()
 *     Description:
 *        Write menu text to a telephone layout
 *        Used for DIAL, DIRECTORY, TOP_8, LIST, and DETAIL layouts.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t dest - Destination device
 *         uint8_t layout - Layout type (IBUS_TEL_LAYOUT_*)
 *         uint8_t function - Function context for input reports (IBUS_TEL_FUNC_*)
 *         uint8_t options - Options bitfield (index | CLEAR | BUFFER | HIGHLIGHT)
 *         char *text - The text to display (use 0x06 as field delimiter)
 *     Returns:
 *         void
 */
void IBusCommandTELMenuText(
    IBus_t *ibus,
    uint8_t dest,
    uint8_t layout,
    uint8_t function,
    uint8_t options,
    char *text
) {
    uint8_t textLength = strlen(text);
    uint8_t msgLength = textLength + 4;
    uint8_t msg[msgLength];
    msg[0] = IBUS_TEL_CMD_MENU_TEXT;
    msg[1] = layout;
    msg[2] = function;
    msg[3] = options;
    if (textLength > 0) {
        memcpy(msg + 4, text, textLength);
    }
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, dest, msg, msgLength);
}

/**
 * IBusCommandTELOnCallIndicator()
 *     Description:
 *        Show or hide the on-call indicator characters on displays.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t show - Show / hide indicator (bool)
 *     Returns:
 *         void
 */
void IBusCommandTELOnCallIndicator(IBus_t *ibus, uint8_t show)
{
    if (show) {
        uint8_t msg[] = {
            IBUS_TEL_CMD_TITLE_TEXT,
            IBUS_TEL_TITLE_ON_CALL,
            0x00,
            IBUS_TEL_CHAR_ON_CALL_LEFT,
            IBUS_TEL_CHAR_ON_CALL_RIGHT
        };
        IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_ANZV, msg, sizeof(msg));
    } else {
        uint8_t msg[] = {IBUS_TEL_CMD_TITLE_TEXT, IBUS_TEL_TITLE_ON_CALL, 0x00};
        IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_ANZV, msg, sizeof(msg));
    }
}

/**
 * IBusCommandTELPropertyText()
 *     Description:
 *        Write property text to a telephone layout (0x24
 *        Used for signal strength bars, call cost, and call duration display.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t dest - Destination device
 *         uint8_t layout - Property layout type (IBUS_TEL_PROP_*)
 *         char *text - The text to display
 *     Returns:
 *         void
 */
void IBusCommandTELPropertyText(
    IBus_t *ibus,
    uint8_t dest,
    uint8_t layout,
    char *text
) {
    uint8_t textLength = strlen(text);
    uint8_t msgLength = textLength + 3;
    uint8_t msg[msgLength];
    msg[0] = IBUS_TEL_CMD_PROPERTY_TEXT;
    msg[1] = layout;
    msg[2] = 0x00;
    if (textLength > 0) {
        memcpy(msg + 3, text, textLength);
    }
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, dest, msg, msgLength);
}

/**
 * IBusCommandTELSignalStrength()
 *     Description:
 *        Display signal strength bars on the telephone info layout.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t dest - Destination device
 *         uint8_t bars - Number of bars to display (0-7)
 *     Returns:
 *         void
 */
void IBusCommandTELSignalStrength(IBus_t *ibus, uint8_t dest, uint8_t bars)
{
    if (bars > 7) {
        bars = 7;
    }
    uint8_t msg[10];
    msg[0] = IBUS_TEL_CMD_PROPERTY_TEXT;
    msg[1] = IBUS_TEL_PROP_SIGNAL_STRENGTH;
    msg[2] = 0x00;
    for (uint8_t i = 0; i < 7; i++) {
        if (i < bars) {
            msg[3 + i] = IBUS_TEL_SIGNAL_BAR_FULL;
        } else {
            msg[3 + i] = IBUS_TEL_SIGNAL_BAR_0;
        }
    }
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, dest, msg, 10);
}

/**
 * IBusCommandTELSMSIcon()
 *     Description:
 *        Control the SMS unread notification icon
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t show - 1 to show icon, 0 to hide icon
 *     Returns:
 *         void
 */
void IBusCommandTELSMSIcon(IBus_t *ibus, uint8_t show)
{
    uint8_t msg[] = {IBUS_TEL_CMD_SMS_ICON, 0x00, show ? 0x01 : 0x00};
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_ANZV, msg, sizeof(msg));
}

/**
 * IBusCommandTELStatus()
 *     Description:
 *        Send the TEL Announcement Message so the car knows we're here
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t status - The status to send to the front display
 *     Returns:
 *         void
 */
void IBusCommandTELStatus(IBus_t *ibus, uint8_t status)
{
    const uint8_t msg[] = {IBUS_TEL_CMD_STATUS, status};
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_ANZV, msg, sizeof(msg));
}

/**
 * IBusCommandTELStatusText()
 *     Description:
 *        Send telephone status text
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *text - The text to send to the front display
 *         uint8_t index - The index to write to
 *     Returns:
 *         void
 */
void IBusCommandTELStatusText(IBus_t *ibus, char *text, uint8_t index)
{
    uint8_t textLength = strlen(text);
    uint8_t statusText[textLength + 3];
    statusText[0] = IBUS_CMD_GT_WRITE_TITLE;
    statusText[1] = 0x80 + index;
    statusText[2] = 0x20;
    memcpy(statusText + 3, text, textLength);
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_ANZV, statusText, sizeof(statusText));
}

/**
 * IBusCommandTELTitleText()
 *     Description:
 *        Write title/heading text to a telephone layout
 *        Used for dial number display, directory contact info, call indicators, etc.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t dest - Destination device
 *         uint8_t layout - Title layout type (IBUS_TEL_TITLE_*)
 *         uint8_t options - Options (IBUS_TEL_TITLE_OPT_UPDATE or IBUS_TEL_TITLE_OPT_SET)
 *         char *text - The text to display
 *     Returns:
 *         void
 */
void IBusCommandTELTitleText(
    IBus_t *ibus,
    uint8_t dest,
    uint8_t layout,
    uint8_t options,
    char *text
) {
    uint8_t textLength = strlen(text);
    uint8_t msgLength = textLength + 3;
    uint8_t msg[msgLength];
    msg[0] = IBUS_TEL_CMD_TITLE_TEXT;
    msg[1] = layout;
    msg[2] = options;
    if (textLength > 0) {
        memcpy(msg + 3, text, textLength);
    }
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, dest, msg, msgLength);
}

/**
 * IBusCommandVMModeSet()
 *     Description:
 *        Send command to Carphonics module
 *        NOTE: Though the VM is targeted, this does not do any interacting
 *        with the factory VM.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         uint8_t enable - 0 = CP off, 1 = CP on, 0xFF = CP don't handle display switch
 *     Returns:
 *         void
 */
void IBusCommandVMModeSet(IBus_t *ibus, uint8_t enable)
{
    uint8_t pkt[] = {
        IBUS_BLUEBUS_CARPHONICS_EXTERNAL_CONTROL,
        IBUS_BLUEBUS_CMD_SET_STATUS,
        enable
    };
    IBusSendCommand(ibus, IBUS_DEVICE_CDC, IBUS_DEVICE_VM, pkt, sizeof(pkt));
}
