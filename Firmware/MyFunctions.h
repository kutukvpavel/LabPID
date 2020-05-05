/*
 * MyFunctions.h
 *
 * Created: 17.08.2018 22:51:40
 *  Author: Павел
 */

#pragma once

#ifdef ARDUINO
	#include <Arduino.h>
#endif // ARDUINO
#include <stdint-gcc.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <math.h>

//Templates
template<typename T, uint8_t s> constexpr uint8_t arraySize(const T (&v)[s]) noexcept;
template<typename T, uint8_t s> constexpr uint8_t arraySize(const T (&v)[s])
{
	return s;
}

//Operators
constexpr uint8_t operator"" _ui8(const unsigned long long int val)
{
	return static_cast<uint8_t>(val);
}

//Macros
#define pgm_read_PGM_P(s_l) (PGM_P)pgm_read_word(&(s_l))
#define pgm_read_EEM_P(s_l) (uint8_t*)pgm_read_word(&(s_l))
#define BV8(bit) (static_cast<uint8_t>(1_ui8 << (bit)))

float decodeFloat(const char* str, uint8_t max_len = 32) __ATTR_PURE__;
float decodeFloat(const char* str, uint8_t start, uint8_t len) __ATTR_PURE__;
int8_t decodeInt(const char* str, uint8_t max_len = 32) __ATTR_PURE__;
int8_t decodeInt(const char* str, uint8_t start, uint8_t len) __ATTR_PURE__;

template<typename T> uint8_t arraySearch(const T* arr, T val, uint8_t len, uint8_t start) __ATTR_PURE__;
template<typename T> uint8_t arraySearch(const T* arr, T val, uint8_t len) __ATTR_PURE__;
template<typename T> uint8_t arraySearch(const T* arr, T val, uint8_t len, uint8_t start, uint8_t exclude) __ATTR_PURE__;
template<typename T> uint8_t arraySearch(const T* arr, T val, uint8_t len)
{
	return arraySearch(arr, val, len, 0);
} // arraySearch
template<typename T> uint8_t arraySearch(const T* arr, T val, uint8_t len, uint8_t start)
{
	for (uint8_t i = start; i < len; ++i) if (arr[i] == val) return i;
	return len;
}
template<typename T> uint8_t arraySearch(const T* arr, T val, uint8_t len, uint8_t start, uint8_t exclude)
{
	uint8_t res = arraySearch(arr, val, len, start);
	if (res == exclude)	res = arraySearch(arr, val, len, exclude);
	return res;
}

uint8_t strcmp_A(const char* str, uint8_t start, uint8_t end, char const* const* arr, uint8_t arr_len) __ATTR_PURE__;
uint8_t strcmp_A(const char* str, uint8_t len, char const* const* arr, uint8_t arr_len) __ATTR_PURE__;
uint8_t strcmp_AP(const char* str, uint8_t start, uint8_t end, char const* const* arr, uint8_t arr_len) __ATTR_PURE__;
uint8_t strcmp_AP(const char* str, uint8_t len, char const* const* arr, uint8_t arr_len) __ATTR_PURE__;

bool strpartcmp(const char* str, uint8_t start, const char* cmp, uint8_t start_cmp, uint8_t len)  __ATTR_PURE__;
bool strpartcmp(const char* str, uint8_t start, const char* cmp, uint8_t len) __ATTR_PURE__;
bool strpartcmp(const char* str, uint8_t start, uint8_t end, const char* cmp, uint8_t start_cmp) __ATTR_PURE__;
bool strpartcmp(const char* str, uint8_t start, uint8_t end, const char* cmp) __ATTR_PURE__;

void substring(const char* str, char* buf, uint8_t start, uint8_t end);

uint8_t strrmv(char* str, uint8_t len);
uint8_t strrmv(char* str, uint8_t start, uint8_t end);

template <typename T> void bitToInt(T* val, const char* str, uint8_t bits = sizeof(T) * 8);
template <typename T> void bitToInt(T* val, const char* str, uint8_t bits/* = sizeof(T) * 8*/)
{
	--bits; *val = 0;
	for (uint8_t i = 0; i <= bits; ++i) *val |= static_cast<T>((*str++ - '0') > 0) << (bits - i);
}

template <typename T> void intToBit(const T val, char* str, uint8_t bits = sizeof(T) * 8);
template <typename T> void intToBit(const T val, char* str, uint8_t bits /*= sizeof(T) * 8*/)
{
	--bits;
	for (uint8_t i = 0; i <= bits; ++i) *str++ = val & static_cast<T>(static_cast<T>(1U) << (bits - i)) ? '1' : '0';
	*str = '\0';
}

template <typename T> T decodeHex(const char* buf) __ATTR_PURE__;
template <typename T> T decodeHex(const char* buf)
{
	T n = 0;
	for (uint8_t c = 0; c < sizeof(T) * 2; c++) {
		n |= static_cast<T>((buf[c] - ((buf[c] < 'A') ? '0' : ('0' + 7))) << ((sizeof(T) * 2U - c - 1U) * 4U));
	}
	return n;
}

uint8_t ASCIIToLower(uint8_t sym) __ATTR_CONST__;
uint16_t utf16ToLowerRus(uint16_t sym) __ATTR_CONST__;
bool isNumeric(const char* str, uint8_t start, uint8_t len) __ATTR_PURE__;
bool isNumeric(const char* str, uint8_t max_len) __ATTR_PURE__;
template <typename T> bool isNumeric(T sym) __ATTR_CONST__;
template <typename T> bool isNumeric(T sym)
{
	return ((sym <= static_cast<T>('9')) && (sym >= static_cast<T>('0')));
}
template <typename T> inline T ASCIIToNum(T sym) __ATTR_CONST__;
template <typename T> T ASCIIToNum(T sym)
{
	return sym - static_cast<T>('0');
}
template <typename T> inline unsigned char NumToASCII(T sym) __ATTR_CONST__;
template <typename T> unsigned char NumToASCII(T sym)
{
	return static_cast<unsigned char>(sym + '0');
}

char *utoa_recursive2 (unsigned val, char *s, unsigned radix) __attribute__((noinline));
char *itoa2 (int val, char *s, int radix) __attribute__((noinline));

#ifndef MEMORY_FREE_H

int freeMemory(void);

#endif // ! MEMORY_FREE_H


//Structures and classes

#define ENABLE_BITFIELD_CHECKS 0
struct BitField
{
	uint8_t* Data;
	#if ENABLE_BITFIELD_CHECKS
	uint8_t Length;
	#endif
	void Set(uint8_t index, bool val)
	{
		#if ENABLE_BITFIELD_CHECKS
		if (index >= Length) return;
		#endif
		val ? this->Data[index / 8] |= BV8(index % 8) : this->Data[index / 8] &= static_cast<uint8_t>(~BV8(index % 8));
	}
	template<typename T> void Populate(T val)
	{
		uint8_t m =
		#if ENABLE_BITFIELD_CHECKS
		Length <?
		#endif
		(sizeof(T) * 8);
		for (uint8_t i = 0; i < m; ++i)	this->Set(i, val & static_cast<T>(static_cast<T>(1) << i));
	}
	bool operator [](uint8_t i) const
	{
		#if ENABLE_BITFIELD_CHECKS
		if (i >= Length) return false;
		#endif
		return this->Data[i / 8] & BV8(i % 8);
	}
	BitField(uint8_t);
	~BitField();
};