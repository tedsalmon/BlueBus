/*
 * IO Mappings for the PIC24
 */
#ifndef IO_MAPPINGS_H
#define IO_MAPPINGS_H
#define IBUS_UART_MODULE 1
#define IBUS_UART_RX_PRIORITY 7
#define IBUS_UART_TX_PRIORITY 5
#define IBUS_UART_RX_PIN_MODE TRISDbits.TRISD11
#define IBUS_UART_RX_PIN LATDbits.LATD11
#define IBUS_UART_RX_RPIN 12
#define IBUS_UART_TX_PIN_MODE TRISDbits.TRISD10
#define IBUS_UART_TX_PIN LATDbits.LATD10
#define IBUS_UART_TX_RPIN 3
#define IBUS_UART_STATUS_MODE TRISDbits.TRISD0
#define IBUS_UART_STATUS PORTDbits.RD0


#define BT_UART_MODULE 2
#define BT_UART_RX_PRIORITY 6
#define BT_UART_TX_PRIORITY 5
#define BT_UART_RX_PIN_MODE TRISGbits.TRISG6
#define BT_UART_RX_PIN LATGbits.LATG6
#define BT_UART_RX_RPIN 21
#define BT_UART_TX_PIN_MODE TRISGbits.TRISG7
#define BT_UART_TX_PIN LATGbits.LATG7
#define BT_UART_TX_RPIN 26

#define SYSTEM_UART_MODULE 3
#define SYSTEM_UART_RX_PRIORITY 3
#define SYSTEM_UART_TX_PRIORITY 4
#define SYSTEM_UART_RX_PIN_MODE TRISDbits.TRISD2
#define SYSTEM_UART_RX_PIN LATDbits.LATD2
#define SYSTEM_UART_RX_RPIN 23
#define SYSTEM_UART_TX_PIN_MODE TRISDbits.TRISD1
#define SYSTEM_UART_TX_PIN LATDbits.LATD1
#define SYSTEM_UART_TX_RPIN 24

#define EEPROM_SPI_MODULE 1
#define EEPROM_CS_PIN PORTDbits.RD8
#define EEPROM_CS_IO_MODE TRISDbits.TRISD8
#define EEPROM_SCK_RPIN 16
#define EEPROM_SDI_PIN_MODE TRISDbits.TRISD9
#define EEPROM_SDI_RPIN 4
#define EEPROM_SDO_RPIN 30

#define BOARD_VERSION_ONE 0
#define BOARD_VERSION_TWO 1

#define BOARD_VERSION_MODE TRISGbits.TRISG8
#define BOARD_VERSION_STATUS PORTGbits.RG8
#define BOARD_VERSION_PD IOCPDGbits.IOCPDG8

#define BT_BOOT_TYPE_MODE TRISEbits.TRISE1
#define BT_BOOT_TYPE LATEbits.LATE1

#define BT_DATA_SEL_MODE TRISEbits.TRISE4
#define BT_DATA_SEL LATEbits.LATE4

#define BT_MFB_MODE TRISEbits.TRISE5
#define BT_MFB LATEbits.LATE5

#define BT_RST_MODE TRISFbits.TRISF0
#define BT_RST LATFbits.LATF0

#define I2C3_SDA_MODE TRISEbits.TRISE7
#define I2C3_SDA LATEbits.LATE7
#define I2C3_SDA_STATUS PORTEbits.RE7

#define I2C3_SCL_MODE TRISEbits.TRISE6
#define I2C3_SCL LATEbits.LATE6
#define I2C3_SCL_STATUS PORTEbits.RE6

#define ON_LED_MODE TRISEbits.TRISE0
#define ON_LED LATEbits.LATE0

#define UART_SEL_BT 0
#define UART_SEL_MCU 1
#define UART_SEL_MODE TRISDbits.TRISD3
#define UART_SEL LATDbits.LATD3

#define IVT_MODE INTCON2bits.AIVTEN
#define IVT_MODE_APP 1
#define IVT_MODE_BOOT 0

#define IBUS_EN_MODE TRISFbits.TRISF1
#define IBUS_EN_STATUS PORTFbits.RF1
#define IBUS_EN LATFbits.LATF1

#define PAM_SHDN_MODE TRISEbits.TRISE3
#define PAM_SHDN LATEbits.LATE3

#define SPDIF_RST_MODE TRISEbits.TRISE2
#define SPDIF_RST LATEbits.LATE2

#define SYS_DTR_MODE TRISDbits.TRISD4
#define SYS_DTR_STATUS PORTDbits.RD4

#define TEL_ON_MODE_V1 TRISBbits.TRISB7
#define TEL_ON_V1 LATBbits.LATB7

#define TEL_MUTE_MODE_V1 TRISEbits.TRISE2
#define TEL_MUTE_V1 LATEbits.LATE2

#define TEL_ON_MODE_V2 TRISFbits.TRISF5
#define TEL_ON_V2 LATFbits.LATF5

#define TEL_MUTE_MODE_V2 TRISFbits.TRISF4
#define TEL_MUTE_V2 LATFbits.LATF4

// UI Events
#define UIEvent_InitiateConnection 96
#define UIEvent_CloseConnection 97

#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 3
#define FIRMWARE_VERSION_PATCH 2

void TrapWait();
#endif /* IO_MAPPINGS_H */
