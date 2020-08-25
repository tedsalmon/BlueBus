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
		uint8_t c = (uint8_t)input[idx];
		unicodeChar = 0 | c;

		// Identify number of bytes to read from the first negative byte
		uint8_t bytesInChar = 0;
		// 11110xxx
		if (c >> 3 == 30) {
			bytesInChar = 3;
		}
		// 1110xxxx
		else if (c >> 4 == 14) {
			bytesInChar = 2;
		}
		// 110xxxx
		else if (c >> 5 == 6) {
			bytesInChar = 1;
		}

		// Identify if we can read more byte
		if (idx + bytesInChar <= strLength) {
			while (bytesInChar != 0) {
				unicodeChar = unicodeChar << 8 | (uint8_t)input[++idx];
				bytesInChar--;
			}
		}
		else {
			idx = strLength;
		}

		if (unicodeChar <= 0x7F) {
			string[strIdx++] = (char)unicodeChar;
		}
		else if (unicodeChar > 0x7F && unicodeChar <= 0xFF) {
			// Convert UTF-8 byte to Unicode then check if it falls within
			// the range of extended ASCII
			uint32_t extendedChar = unicodeChar - 0xC2C0;
			if (extendedChar < 0xFF) {
				string[strIdx++] = (char)extendedChar;
			}
		}
		else if (unicodeChar > 0xFF) {
			const char* transStr = TransliterateUnicodeToExtendedASCII(unicodeChar);
			transStrLength = strlen(transStr);
			for (transIdx = 0; transIdx < transStrLength; transIdx++) {
				string[strIdx++] = (char)transStr[transIdx];
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
 * TransliterateUnicodeToExtendedASCII()
 *     Description:
 *         Transliterates Unicode character to the corresponding ASCII string.
 *     Params:
 *         uint32_t - Representation of the Unicode character
 *     Returns:
 *         const char* - Corresponding Extended ASCII characters
 */
const char* TransliterateUnicodeToExtendedASCII(uint32_t input) {
	switch (input) {
	case 0xCA80: return "R"; break; // LATIN LETTER SMALL CAPITAL R
	case 0xD081: return "Yo"; break; // CYRILLIC CAPITAL LETTER IO
	case 0xD084: return "E"; break; // CYRILLIC CAPITAL LETTER UKRAINIAN IE
	case 0xD086: return "I"; break; // CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
	case 0xD090: return "A"; break; // CYRILLIC CAPITAL LETTER A
	case 0xD091: return "B"; break; // CYRILLIC CAPITAL LETTER BE
	case 0xD092: return "V"; break; // CYRILLIC CAPITAL LETTER VE
	case 0xD093: return "G"; break; // CYRILLIC CAPITAL LETTER GHE
	case 0xD094: return "D"; break; // CYRILLIC CAPITAL LETTER DE
	case 0xD095: return "Ye"; break; // CYRILLIC CAPITAL LETTER IE
	case 0xD096: return "Zh"; break; // CYRILLIC CAPITAL LETTER ZHE
	case 0xD097: return "Z"; break; // CYRILLIC CAPITAL LETTER ZE
	case 0xD098: return "I"; break; // CYRILLIC CAPITAL LETTER I
	case 0xD099: return "Y"; break; // CYRILLIC CAPITAL LETTER SHORT I
	case 0xD09A: return "K"; break; // CYRILLIC CAPITAL LETTER KA
	case 0xD09B: return "L"; break; // CYRILLIC CAPITAL LETTER EL
	case 0xD09C: return "M"; break; // CYRILLIC CAPITAL LETTER EM
	case 0xD09D: return "N"; break; // CYRILLIC CAPITAL LETTER EN
	case 0xD09E: return "O"; break; // CYRILLIC CAPITAL LETTER O
	case 0xD09F: return "P"; break; // CYRILLIC CAPITAL LETTER PE
	case 0xD0A0: return "R"; break; // CYRILLIC CAPITAL LETTER ER
	case 0xD0A1: return "S"; break; // CYRILLIC CAPITAL LETTER ES
	case 0xD0A2: return "T"; break; // CYRILLIC CAPITAL LETTER TE
	case 0xD0A3: return "U"; break; // CYRILLIC CAPITAL LETTER U
	case 0xD0A4: return "F"; break; // CYRILLIC CAPITAL LETTER EF
	case 0xD0A5: return "Kh"; break; // CYRILLIC CAPITAL LETTER HA
	case 0xD0A6: return "Ts"; break; // CYRILLIC CAPITAL LETTER TSE
	case 0xD0A7: return "Ch"; break; // CYRILLIC CAPITAL LETTER CHE
	case 0xD0A8: return "Sh"; break; // CYRILLIC CAPITAL LETTER SHA
	case 0xD0A9: return "Shch"; break; // CYRILLIC CAPITAL LETTER SHCHA
	case 0xD0AA: return "\""; break; // CYRILLIC CAPITAL LETTER HARD SIGN
	case 0xD0AB: return "Y"; break; // CYRILLIC CAPITAL LETTER YERU
	case 0xD0AC: return "'"; break; // CYRILLIC CAPITAL LETTER SOFT SIGN
	case 0xD0AD: return "E"; break; // CYRILLIC CAPITAL LETTER E
	case 0xD0AE: return "Yu"; break; // CYRILLIC CAPITAL LETTER YU
	case 0xD0AF: return "Ya"; break; // CYRILLIC CAPITAL LETTER YA
	case 0xD0B0: return "a"; break; // CYRILLIC SMALL LETTER A
	case 0xD0B1: return "b"; break; // CYRILLIC SMALL LETTER BE
	case 0xD0B2: return "v"; break; // CYRILLIC SMALL LETTER VE
	case 0xD0B3: return "g"; break; // CYRILLIC SMALL LETTER GHE
	case 0xD0B4: return "d"; break; // CYRILLIC SMALL LETTER DE
	case 0xD0B5: return "ye"; break; // CYRILLIC SMALL LETTER IE
	case 0xD0B6: return "zh"; break; // CYRILLIC SMALL LETTER ZHE
	case 0xD0B7: return "z"; break; // CYRILLIC SMALL LETTER ZE
	case 0xD0B8: return "i"; break; // CYRILLIC SMALL LETTER I
	case 0xD0B9: return "y"; break; // CYRILLIC SMALL LETTER SHORT I
	case 0xD0BA: return "k"; break; // CYRILLIC SMALL LETTER KA
	case 0xD0BB: return "l"; break; // CYRILLIC SMALL LETTER EL
	case 0xD0BC: return "m"; break; // CYRILLIC SMALL LETTER EM
	case 0xD0BD: return "n"; break; // CYRILLIC SMALL LETTER EN
	case 0xD0BE: return "o"; break; // CYRILLIC SMALL LETTER O
	case 0xD0BF: return "p"; break; // CYRILLIC SMALL LETTER PE
	case 0xD180: return "r"; break; // CYRILLIC SMALL LETTER ER
	case 0xD181: return "s"; break; // CYRILLIC SMALL LETTER ES
	case 0xD182: return "t"; break; // CYRILLIC SMALL LETTER TE
	case 0xD183: return "u"; break; // CYRILLIC SMALL LETTER U
	case 0xD184: return "f"; break; // CYRILLIC SMALL LETTER EF
	case 0xD185: return "kh"; break; // CYRILLIC SMALL LETTER HA
	case 0xD186: return "ts"; break; // CYRILLIC SMALL LETTER TSE
	case 0xD187: return "ch"; break; // CYRILLIC SMALL LETTER CHE
	case 0xD188: return "sh"; break; // CYRILLIC SMALL LETTER SHA
	case 0xD189: return "shch"; break; // CYRILLIC SMALL LETTER SHCHA
	case 0xD18A: return "\""; break; // CYRILLIC SMALL LETTER HARD SIGN
	case 0xD18B: return "y"; break; // CYRILLIC SMALL LETTER YERU
	case 0xD18C: return "'"; break; // CYRILLIC SMALL LETTER SOFT SIGN
	case 0xD18D: return "e"; break; // CYRILLIC SMALL LETTER E
	case 0xD18E: return "yu"; break; // CYRILLIC SMALL LETTER YU
	case 0xD18F: return "ya"; break; // CYRILLIC SMALL LETTER YA
	case 0xD191: return "yo"; break; // CYRILLIC SMALL LETTER IO
	case 0xD194: return "e"; break; // CYRILLIC SMALL LETTER UKRAINIAN IE
	case 0xD196: return "i"; break; // CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
	case 0xE28098: return "'"; break; // LEFT SINGLE QUOTATION MARK
	case 0xE28099: return "'"; break; // RIGHT SINGLE QUOTATION MARK
	default: return ""; break;
	}
}
