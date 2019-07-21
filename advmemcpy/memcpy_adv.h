#pragma once

#include <windows.h>

void* memcpy_thread_initialize(size_t threads);
void  memcpy_thread_set_memcpy(void* (*memcpy)(void*, const void*, size_t));
void  memcpy_thread_env(void* env);
void* memcpy_thread(void* to, void* from, size_t size);
void  memcpy_thread_finalize(void* env);

static inline void* memcpy_movsq(void* to, void* from, size_t size)
{
	if (size % 8 == 0) {
		__movsq(reinterpret_cast<unsigned long long*>(to), reinterpret_cast<unsigned long long*>(from),
		        size / 8);
	} else if (size % 4 == 0) {
		__movsd(reinterpret_cast<unsigned long*>(to), reinterpret_cast<unsigned long*>(from), size / 4);
	} else if (size % 2 == 0) {
		__movsw(reinterpret_cast<unsigned short*>(to), reinterpret_cast<unsigned short*>(from), size / 2);
	} else {
		__movsb(reinterpret_cast<unsigned char*>(to), reinterpret_cast<unsigned char*>(from), size);
	}
	return to;
};
static inline void* memcpy_movsd(void* to, void* from, size_t size)
{
	if (size % 4 == 0) {
		__movsd(reinterpret_cast<unsigned long*>(to), reinterpret_cast<unsigned long*>(from), size / 4);
	} else if (size % 2 == 0) {
		__movsw(reinterpret_cast<unsigned short*>(to), reinterpret_cast<unsigned short*>(from), size / 2);
	} else {
		__movsb(reinterpret_cast<unsigned char*>(to), reinterpret_cast<unsigned char*>(from), size);
	}
	return to;
};
static inline void* memcpy_movsw(void* to, void* from, size_t size)
{
	if (size % 2 == 0) {
		__movsw(reinterpret_cast<unsigned short*>(to), reinterpret_cast<unsigned short*>(from), size / 2);
	} else {
		__movsb(reinterpret_cast<unsigned char*>(to), reinterpret_cast<unsigned char*>(from), size);
	}
	return to;
};
static inline void* memcpy_movsb(void* to, void* from, size_t size)
{
	__movsb(reinterpret_cast<unsigned char*>(to), reinterpret_cast<unsigned char*>(from), size);
	return to;
};
