#include "MyFunctions.h"

BitField::BitField(uint8_t length)
#if ENABLE_BITFIELD_CHECKS
: Length(length)
#endif
{
	Data = new uint8_t[length / 8 + (length % 8 == 0) ? 0 : 1];
}
BitField::~BitField()
{
	delete[] Data;
}

float decodeFloat(const char* str, uint8_t max_len)
{
	return decodeFloat(str, 0, strnlen(str, max_len));
}
float decodeFloat(const char* str, uint8_t start, uint8_t len) //Decode float from string (format: -XXX.XXX), smth like -X or -.X work too
{
	if (len == 0) return NAN;
	static const float mult[] PROGMEM = { 100, 10, 1, 0.1, 0.01, 0.001, 0.0001 };
	str += start;
	bool neg = false;
	if (str[0] == '-') { neg = true; str++; } //Get rid of signs
	else { if (str[0] == '+') str++; };
	uint8_t p, i;
	for (p = 0; p < len; p++) if (str[p] == '.') break; //Find decimal point (if present)
	for (; p > 3; p--) str++; //Truncate string front
	if (len - p > 4) len = p + 4; //Truncate string end
	float b = 0;
	for (i = 0; i < len; i++) { //Iterate through each digit and build the number using multipliers
		if (i == p) continue; //Skip point
		b += (str[i] - '0') * pgm_read_float(&(mult[((i < p) ? 3 : 2) + i - p]));
	}
	if (neg) b *= -1;
	return b;
}
int8_t decodeInt(const char* str, uint8_t max_len)
{
	return static_cast<int8_t>(decodeFloat(str));
}
int8_t decodeInt(const char* str, uint8_t start, uint8_t len)
{
	return static_cast<int8_t>(decodeFloat(str, start, len));
} // DECODE NUMBER

uint8_t strcmp_A(const char* str, uint8_t start, uint8_t end, char const* const* arr, uint8_t arr_len)
{
	str += start;
	end -= start;
	return strcmp_A(str, end, arr, arr_len);
}
uint8_t strcmp_A(const char* str, uint8_t len, char const* const* arr, uint8_t arr_len)
{
	for (uint8_t i = 0; i < arr_len; i++) if (strncmp(str, arr[i], len) == 0) return i;
	return UINT8_MAX;
} // strcmp_A

uint8_t strcmp_AP(const char* str, uint8_t start, uint8_t end, char const* const* arr, uint8_t arr_len)
{
	str += start;
	end -= start;
	return strcmp_AP(str, end, arr, arr_len);
}
uint8_t strcmp_AP(const char* str, uint8_t len, char const* const* arr, uint8_t arr_len)
{
	for (uint8_t i = 0; i < arr_len; i++) if (strncmp_P(str, pgm_read_PGM_P(arr[i]), len) == 0) return i;
	return UINT8_MAX;
} // strcmp_AP

bool strpartcmp(const char* str, uint8_t start, const char* cmp, uint8_t start_cmp, uint8_t len)
{
	bool res = true;
	str += start;
	cmp += start_cmp;
	for (uint8_t i = 0; i < len; i++) { if (*str++ != *cmp++) { res = false; break; }; };
	return res;
}
bool strpartcmp(const char* str, uint8_t start, const char* cmp, uint8_t len)
{
	return strpartcmp(str, start, start + len, cmp, 0);
}
bool strpartcmp(const char* str, uint8_t start, uint8_t end, const char* cmp, uint8_t start_cmp)
{
	return strpartcmp(str, start, cmp, start_cmp, end - start);
}
bool strpartcmp(const char* str, uint8_t start, uint8_t end, const char* cmp)
{
	return strpartcmp(str, start, cmp, 0, end - start);
} // strpartcmp

//end is exclusive, start is inclusive
void substring(const char* str, char* buf, uint8_t start, uint8_t end)
{
	end -= start;
	str += start;
	for (uint8_t i = 0; i < end; i++) *buf++ = *str++;
	*buf = '\0';
} // substring

// Returns the length of new string
uint8_t strrmv(char* str, uint8_t start, uint8_t end)
{
	end -= start;
	str += start;
	return start + strrmv(str, end);
}
uint8_t strrmv(char* str, uint8_t len)
{
	uint8_t e = strlen(str);
	for (uint8_t i = 0; i < e; i++) {
		if ((i + len) < e) { *str = str[len]; str++; }
		else { *str++ = '\0'; };
	}
	*str = '\0';
	return e - len;
}

uint16_t utf16ToLowerRus(uint16_t sym)
{
	if ((sym < 0x430U) && (sym > 0x40FU)) sym += 32U;
	return sym;
}
uint8_t ASCIIToLower(uint8_t sym)
{
	if ((sym < 0x5BU) && (sym > 0x40U)) sym += 32U;
	return sym;
}

bool isNumeric(const char* str, uint8_t start, uint8_t len)
{
	str += start;
	const char* end = str + len;
	while (str < end) if (!isNumeric(*str++)) return false;
	return true;
}
bool isNumeric(const char* str, uint8_t max_len)
{
	return isNumeric(str, 0_ui8, strnlen(str, max_len));
}

char *utoa_recursive2 (unsigned val, char *s, unsigned radix)
{
	int c;
	if (val >= radix)
	s = utoa_recursive2 (val / radix, s, radix);
	c = val % radix;
	c += (c < 10 ? '0' : 'a' - 10);
	*s++ = c;
	return s;
}

char *itoa2 (int val, char *s, int radix)
{
	if (radix < 2 || radix > 36) {
		s[0] = 0;
		} else {
		char *p = s;
		if (radix == 10 && val < 0) {
			val = -val;
			*p++ = '-';
		}
		*utoa_recursive2 (val, p, radix) = 0;
	}
	return s;
}

#ifndef MEMORY_FREE_H

int freeMemory()
{
	extern unsigned int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

#endif // 
