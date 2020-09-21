/*
 * File:   utils.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Helper utils that may be useful in more than one place
 */
#include "utils.h"

/* Hold a pin to register map for all programmable output pins */
static uint16_t *ROPR_PINS[] = {
    GET_RPOR(0),
    GET_RPOR(0),
    GET_RPOR(1),
    GET_RPOR(1),
    GET_RPOR(2),
    GET_RPOR(2),
    GET_RPOR(3),
    GET_RPOR(3),
    GET_RPOR(4),
    GET_RPOR(4),
    GET_RPOR(5),
    GET_RPOR(5),
    GET_RPOR(6),
    GET_RPOR(6),
    GET_RPOR(7),
    GET_RPOR(7),
    GET_RPOR(8),
    GET_RPOR(8),
    GET_RPOR(9),
    GET_RPOR(9),
    GET_RPOR(10),
    GET_RPOR(10),
    GET_RPOR(11),
    GET_RPOR(11),
    GET_RPOR(12),
    GET_RPOR(12),
    GET_RPOR(13),
    GET_RPOR(13),
    GET_RPOR(14),
    GET_RPOR(14),
    GET_RPOR(15),
    GET_RPOR(15),
    GET_RPOR(16),
    GET_RPOR(16),
    GET_RPOR(17),
    GET_RPOR(17),
    GET_RPOR(18),
    GET_RPOR(18)
};

UtilsAbstractDisplayValue_t UtilsDisplayValueInit(char *text, uint8_t status)
{
    UtilsAbstractDisplayValue_t value;
    strncpy(value.text, text, UTILS_DISPLAY_TEXT_SIZE - 1);
    value.index = 0;
    value.timeout = 0;
    value.status = status;
    value.length = strlen(text);
    return value;
}

/**
 * UtilsNormalizeText()
 *     Description:
 *         Unescape characters and convert them from UTF-8 to their Unicode
 *         bytes. This is to support extended ASCII.
 *     Params:
 *         char *string - The subject
 *         const char *input - The string to copy from
 *     Returns:
 *         void
 */
void UtilsNormalizeText(char *string, const char *input)
{
    uint16_t idx;
    uint16_t strIdx = 0;
    uint32_t unicodeChar;

    uint8_t transIdx;
    uint8_t transStrLength;

    uint16_t strLength = strlen(input);

    for (idx = 0; idx < strLength; idx++) {
        uint8_t c = (uint8_t) input[idx];
        unicodeChar = 0 | c;

        // Identify number of bytes to read from the first negative byte
        uint8_t bytesInChar = 0;

        if (c >> 3 == 30) { // 11110xxx
            bytesInChar = 3;
        } else if (c >> 4 == 14) { // 1110xxxx
            bytesInChar = 2;
        } else if (c >> 5 == 6) { // 110xxxx
            bytesInChar = 1;
        }

        // Identify if we can read more byte
        if (idx + bytesInChar <= strLength) {
            while (bytesInChar != 0) {
                unicodeChar = unicodeChar << 8 | (uint8_t) input[++idx];
                bytesInChar--;
            }
        } else {
            idx = strLength;
        }

        if (unicodeChar <= 0x7F) {
            string[strIdx++] = (char) unicodeChar;
        } else if (unicodeChar >= 0xC280 && unicodeChar <= 0xC3BF) {
            // Convert UTF-8 byte to Unicode then check if it falls within
            // the range of extended ASCII
            uint32_t extendedChar = (unicodeChar & 0xFF) + ((unicodeChar >> 8) - 0xC2) * 64;
            if (extendedChar < 0xFF) {
                string[strIdx++] = (char) extendedChar;
            }
        } else if (unicodeChar > 0xFF) {
            char * transStr = UtilsTransliterateUnicodeToASCII(unicodeChar);
            transStrLength = strlen(transStr);
            for (transIdx = 0; transIdx < transStrLength; transIdx++) {
                string[strIdx++] = (char) transStr[transIdx];
            }
        }
    }
    string[strIdx] = '\0';
}

/**
 * UtilsRemoveSubstring()
 *     Description:
 *         Remove the given substring from the given subject
 *     Params:
 *         char *string - The subject
 *         const char *trash - The substring to remove
 *     Returns:
 *         void
 */
void UtilsRemoveSubstring(char *string, const char *trash)
{
    uint16_t removeLength = strlen(trash);
    while ((string = strstr(string, trash))) {
        memmove(string, string + removeLength, 1 + strlen(string + removeLength));
    }
}

/**
 * UtilsReset()
 *     Description:
 *         Reset the MCU
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void UtilsReset()
{
    __asm__ volatile ("RESET");
}

/**
 * UtilsSetRPORMode()
 *     Description:
 *         Set the mode of a programmable output pin
 *     Params:
 *         uint8_t pin - The pin to set
 *         uint8_t mode - The mode to set the given pin to
 *     Returns:
 *         void
 */
void UtilsSetRPORMode(uint8_t pin, uint16_t mode)
{
    if ((pin % 2) == 0) {
        uint16_t msb = *ROPR_PINS[pin] >> 8;
        // Set the least significant bits for the even pin number
        *ROPR_PINS[pin] = (msb << 8) + mode;
    } else {
        uint16_t lsb = *ROPR_PINS[pin] & 0xFF;
        // Set the least significant bits of the register for the odd pin number
        *ROPR_PINS[pin] = (mode << 8) + lsb;
    }
}

/**
 * UtilsStrToHex()
 *     Description:
 *         Convert a string to a octal
 *     Params:
 *         char *string - The subject
 *     Returns:
 *         uint8_t The unsigned char
 */
unsigned char UtilsStrToHex(char *string)
{
    char *ptr;
    return (unsigned char) strtol(string, &ptr, 16);
}


/**
 * UtilsStrToInt()
 *     Description:
 *         Convert a string to an integer
 *     Params:
 *         char *string - The subject
 *     Returns:
 *         uint8_t The Unsigned 8-bit integer representation
 */
uint8_t UtilsStrToInt(char *string)
{
    char *ptr;
    return (uint8_t) strtol(string, &ptr, 10);
}

/**
 * UtilsStricmp()
 *     Description:
 *         Case-Insensitive string comparison 
 *     Params:
 *         const char *string - The subject
 *         const char *compare - The string to compare the subject against
 *     Returns:
 *         int8_t -
 *             Negative 1 when string is less than compare
 *             Zero when string matches compare
 *             Positive 1 when string is greater than compare
 */
int8_t UtilsStricmp(const char *string, const char *compare)
{
    int8_t result;
    while(!(result = toupper(*string) - toupper(*compare)) && *string) {
        string++;
        compare++;
    }
    return result;
}

/**
 * UtilsTransliterateUnicodeToExtendedASCII()
 *     Description:
 *         Transliterates Unicode character to the corresponding ASCII string
 *     Params:
 *         uint32_t input - Representation of the Unicode character
 *     Returns:
 *         char * - Corresponding Extended ASCII characters
 */
char * UtilsTransliterateUnicodeToASCII(uint32_t input)
{
    switch (input) {
        case UTILS_CHAR_LATIN_SMALL_CAPITAL_R:
            return "R";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_CAPITAL_IO:
            return "Yo";
            break;
        case UTILS_CHAR_CYRILLIC_UA_CAPITAL_IE:
            return "E";
            break;
        case UTILS_CHAR_CYRILLIC_BYELORUSSIAN_UA_CAPITAL_I:
            return "I";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_A:
            return "A";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_BE:
            return "B";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_VE:
            return "V";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_GHE:
            return "G";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_DE:
            return "D";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YE:
            return "Ye";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ZHE:
            return "Zh";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ZE:
            return "Z";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_I:
            return "I";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHORT_I:
            return "Y";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_KA:
            return "K";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EL:
            return "L";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EM:
            return "M";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EN:
            return "N";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_O:
            return "O";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_PE:
            return "P";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ER:
            return "R";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ES:
            return "S";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_TE:
            return "T";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_U:
            return "U";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EF:
            return "F";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_HA:
            return "Kh";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_TSE:
            return "Ts";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_CHE:
            return "Ch";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHA:
            return "Sh";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SCHA:
            return "Shch";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_HARD_SIGN:
            return "\"";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YERU:
            return "Y";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SOFT_SIGN:
            return "'";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_E:
            return "E";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YU:
            return "Yu";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YA:
            return "Ya";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_A:
            return "a";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_BE:
            return "b";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_VE:
            return "v";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_GHE:
            return "g";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_DE:
            return "d";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_IE:
            return "ye";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ZHE:
            return "zh";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ZE:
            return "z";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_I:
            return "i";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHORT_I:
            return "y";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_KA:
            return "k";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EL:
            return "l";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EM:
            return "m";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EN:
            return "n";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_O:
            return "o";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_PE:
            return "p";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ER:
            return "r";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ES:
            return "s";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_TE:
            return "t";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_U:
            return "u";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EF:
            return "f";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_HA:
            return "kh";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_TSE:
            return "ts";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_CHE:
            return "ch";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHA:
            return "sh";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHCHA:
            return "shch";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_LEFT_HARD_SIGN:
            return "\"";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YERU:
            return "y";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SOFT_SIGN:
            return "'";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_E:
            return "e";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YU:
            return "yu";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YA:
            return "ya";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_IO:
            return "yo";
            break;
        case UTILS_CHAR_CYRILLIC_UA_SMALL_IE:
            return "e";
            break;
        case UTILS_CHAR_CYRILLIC_BYELORUSSIAN_UA_SMALL_I:
            return "i";
            break;
        case UTILS_CHAR_LEFT_SINGLE_QUOTATION_MARK:
            return "'";
            break;
        case UTILS_CHAR_RIGHT_SINGLE_QUOTATION_MARK:
            return "'";
            break;
        default:
            return "";
            break;
    }
}
