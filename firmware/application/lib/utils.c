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
    uint8_t bytesInChar = 0;
    uint32_t unicodeChar;

    uint8_t transIdx;
    uint8_t transStrLength;

    uint16_t strLength = strlen(input);

    for (idx = 0; idx < strLength; idx++) {
        uint8_t c = (uint8_t) input[idx];

        if (c == 0x5C) {
            if (idx + 2 <= strLength) {
                char buf[] = { (uint8_t) input[idx + 1], (uint8_t) input[idx + 2], '\0' };
                c = UtilsStrToHex(buf);

                idx += 2;
            } else {
                idx = strLength;
                continue;
            }
        }

        if (bytesInChar == 0) {
            unicodeChar = 0 | c;

            // Identify number of bytes to read from the first negative byte
            if (c >> 3 == 30) { // 11110xxx
                bytesInChar = 3;
                continue;
            } else if (c >> 4 == 14) { // 1110xxxx
                bytesInChar = 2;
                continue;
            } else if (c >> 5 == 6) { // 110xxxx
                bytesInChar = 1;
                continue;
            }
        }

        if (bytesInChar > 0) {
            unicodeChar = unicodeChar << 8 | c;
            bytesInChar--;
        }

        if (bytesInChar != 0) {
            continue;
        }

        if (unicodeChar <= 0x7F) {
            string[strIdx++] = (char) unicodeChar;
        } else if (unicodeChar >= 0xC2A1 && unicodeChar <= 0xC2BF) {
            // Convert UTF-8 byte to Unicode then check if it falls within
            // the range of extended ASCII
            uint32_t extendedChar = (unicodeChar & 0xFF) + ((unicodeChar >> 8) - 0xC2) * 64;
            if (extendedChar < 0xFF) {
                string[strIdx++] = (char) extendedChar;
            }
        } else if (unicodeChar > 0xC2BF) {
            char * transStr = UtilsTransliterateUnicodeToASCII(unicodeChar);
            transStrLength = strlen(transStr);
            if (transStrLength != 0) {
                for (transIdx = 0; transIdx < transStrLength; transIdx++) {
                    string[strIdx++] = (char)transStr[transIdx];
                }
            } else {
                char transChar = UtilsTranslateCyrillicUnicodeToASCII(unicodeChar);
                if (transChar != 0) {
                    string[strIdx++] = transChar;
                }
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
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_TILDE:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_RING_ABOVE:
            return "A";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_AE:
            return "Ae";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_C_WITH_CEDILLA:
            return "C";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_DIAERESIS:
            return "E";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_DIAERESIS:
            return "I";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_ETH:
            return "Eth";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_N_WITH_TILDE:
            return "N";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_TILDE:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_STROKE:
            return "O";
            break;
        case UTILS_CHAR_MULTIPLICATION_SIGN:
            return "x";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_DIAERESIS:
            return "U";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_Y_WITH_ACUTE:
            return "Y";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_THORN:
            return "Th";
            break;
        case UTILS_CHAR_LATIN_SMALL_SHARP_S:
            return "ss";
            break;
        case UTILS_CHAR_LATIN_SMALL_A_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_TILDE:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_RING_ABOVE:
            return "a";
            break;
        case UTILS_CHAR_LATIN_SMALL_AE:
            return "ae";
            break;
        case UTILS_CHAR_LATIN_SMALL_C_WITH_CEDILLA:
            return "c";
            break;
        case UTILS_CHAR_LATIN_SMALL_E_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_E_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_E_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_E_WITH_DIAERESIS:
            return "e";
            break;
        case UTILS_CHAR_LATIN_SMALL_I_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_I_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_I_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_I_WITH_DIAERESIS:
            return "i";
            break;
        case UTILS_CHAR_LATIN_SMALL_ETH:
            return "eth";
            break;
        case UTILS_CHAR_LATIN_SMALL_N_WITH_TILDE:
            return "n";
            break;
        case UTILS_CHAR_LATIN_SMALL_O_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_TILDE:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_STROKE:
            return "o";
            break;
        case UTILS_CHAR_DIVISION_SIGN:
            return "%";
            break;
        case UTILS_CHAR_LATIN_SMALL_U_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_U_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_U_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_U_WITH_DIAERESIS:
            return "u";
            break;
        case UTILS_CHAR_LATIN_SMALL_Y_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_Y_WITH_DIAERESIS:
            return "y";
            break;
        case UTILS_CHAR_LATIN_SMALL_THORN:
            return "th";
            break;
        case UTILS_CHAR_LATIN_SMALL_CAPITAL_R:
            return "R";
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

/**
 * TransliterateUnicodeToExtendedASCII()
 *     Description:
 *         Translate Cyrillic Unicode character to the corresponding Extended ASCII char.
 *     Params:
 *         uint32_t - Representation of the Cyrillic Unicode character
 *     Returns:
 *         char - Corresponding Extended ASCII characters
 */
char UtilsTranslateCyrillicUnicodeToASCII(uint32_t input) 
{
    switch (input) {
        case UTILS_CHAR_CYRILLIC_BY_UA_CAPITAL_I:
        case UTILS_CHAR_CYRILLIC_CAPITAL_YI:
            return 'I';
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_A:
            return 192;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_BE:
            return 193;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_VE:
            return 194;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_GHE:
            return 195;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_DE:
            return 196;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_IO:
        case UTILS_CHAR_CYRILLIC_UA_CAPITAL_IE:
        case UTILS_CHAR_CYRILLIC_CAPITAL_YE:
            return 197;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ZHE:
            return 198;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ZE:
            return 199;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_I:
            return 200;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHORT_I:
            return 201;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_KA:
            return 202;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EL:
            return 203;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EM:
            return 204;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EN:
            return 205;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_O:
            return 206;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_PE:
            return 207;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ER:
            return 208;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ES:
            return 209;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_TE:
            return 210;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_U:
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHORT_U:
            return 211;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EF:
            return 212;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_HA:
            return 213;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_TSE:
            return 214;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_CHE:
            return 215;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHA:
            return 216;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SCHA:
            return 217;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_HARD_SIGN:
            return 218;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YERU:
            return 219;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SOFT_SIGN:
            return 220;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_E:
            return 221;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YU:
            return 222;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YA:
            return 223;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_A:
            return 224;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_BE:
            return 225;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_VE:
            return 226;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_GHE:
            return 227;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_DE:
            return 228;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_IE:
        case UTILS_CHAR_CYRILLIC_SMALL_IO:
        case UTILS_CHAR_CYRILLIC_UA_SMALL_IE:
            return 229;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ZHE:
            return 230;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ZE:
            return 231;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_I:
            return 232;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHORT_I:
            return 233;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_KA:
            return 234;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EL:
            return 235;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EM:
            return 236;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EN:
            return 237;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_O:
            return 238;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_PE:
            return 239;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ER:
            return 240;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ES:
            return 241;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_TE:
            return 242;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_U:
        case UTILS_CHAR_CYRILLIC_SMALL_SHORT_U:
            return 243;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EF:
            return 244;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_HA:
            return 245;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_TSE:
            return 246;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_CHE:
            return 247;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHA:
            return 248;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHCHA:
            return 249;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_LEFT_HARD_SIGN:
            return 250;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YERU:
            return 251;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SOFT_SIGN:
            return 252;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_E:
            return 253;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YU:
            return 254;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YA:
            return 255;
            break;
        case UTILS_CHAR_CYRILLIC_BY_UA_SMALL_I:
        case UTILS_CHAR_CYRILLIC_SMALL_YI:
            return 'i';
            break;
        default:
            return 0;
            break;
    }
}
