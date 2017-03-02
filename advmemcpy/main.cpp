// advmemcpy.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <cstring>
#include <chrono>
#include <iostream>
#include <memory.h>
#include <random>
#include <stdint.h>
#include <vector>
#include <thread>
#include <sstream>

#include "memcpy_adv.h"

#define memcpy_c memcpy
#define memcpy_cpp std::memcpy

struct TimingInfo {
	uint64_t
		timeBegin,
		timeEnd,
		timeTotal;
	uint64_t
		callAmount = 0,
		minimumCallTime = UINT64_MAX,
		maximumCallTime = 0,
		totalCallTime = 0,
		validCalls = 0;
	double_t
		averageCallTime = 0.0;
};

class Timing {
private:
	TimingInfo* ti = nullptr;

public:
	Timing(TimingInfo* p_ti) {
		ti = p_ti;
		ti->timeBegin = std::chrono::nanoseconds(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	}
	~Timing() {
		ti->timeEnd = std::chrono::nanoseconds(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		ti->timeTotal = ti->timeEnd - ti->timeBegin;
		ti->callAmount++;
		ti->totalCallTime += ti->timeTotal;

		if (ti->timeTotal < ti->minimumCallTime)
			ti->minimumCallTime = ti->timeTotal;
		if (ti->timeTotal > ti->maximumCallTime)
			ti->maximumCallTime = ti->timeTotal;

		ti->averageCallTime = (double_t)ti->totalCallTime / (double_t)ti->callAmount;
	}
};

void formattedPrint(TimingInfo* ti, const char* name) {
	if (ti == nullptr) {
		std::cout <<
			"| Name             | Calls    | Valid    | Average Time | Minimum Time | Maximum Time |" << std::endl;
		std::cout <<
			"+------------------+----------+----------+--------------+--------------+--------------+" << std::endl;
	} else {
		std::vector<char> buf(1024);
		sprintf_s(buf.data(), buf.size(),
			"| %-16s | %8lld | %8lld | %9lld ns | %9lld ns | %9lld ns |",
			name,
			ti->callAmount,
			ti->validCalls,
			(uint64_t)ti->averageCallTime,
			ti->minimumCallTime,
			ti->maximumCallTime);
		std::cout << buf.data() << std::endl;
	}
}

#ifdef _WIN32
#include "windows.h"
#endif

int main(int argc, char** argv) {
	size_t iterations = 10000;
	size_t memorysize = 3840 * 2160 * 4; // A single 3840x2160 RGBA frame

	if (argc > 0) {
		std::stringstream arg0(argv[0]);
		arg0 >> memorysize;
	}

#ifdef _WIN32
	//HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	//SetConsoleActiveScreenBuffer(hConsole);
	//SetStdHandle(STD_OUTPUT_HANDLE, hConsole);
	//SetStdHandle(STD_ERROR_HANDLE, hConsole);
	//SetStdHandle(STD_INPUT_HANDLE, hConsole);
	//freopen("CONIN$", "r", stdin);
	//freopen("CONOUT$", "w", stdout);
	//freopen("CONOUT$", "w", stderr);
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	std::cout << "Initializing..." << std::endl;
	srand((unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count());
	std::vector<char> memory_from(memorysize);
	std::vector<char> memory_to(memorysize);
	for (size_t p = 0; p < memorysize; p++) {
		// Ensure memory is randomized.
		memory_from[p] = (char)rand();
	}
	std::memset(memory_to.data(), 0, memory_to.size());

	const size_t max_threads = 8;
	TimingInfo
		ti_c,
		ti_cpp;
	TimingInfo ti_threads[max_threads + 1];
	void *ti_threads_env[max_threads + 1];

	for (size_t t = 2; t <= max_threads; t++) {
		ti_threads_env[t] = memcpy_thread_initialize(t);
	}

#ifdef _WIN32
	COORD cp; cp.X = 0; cp.Y = 0;
	SetConsoleCursorPosition(hConsole, cp);
#endif
	formattedPrint(nullptr, nullptr);
	for (size_t it = 0; it <= iterations; it++) {
#ifdef _WIN32
		cp.X = 0; cp.Y = 2;
		SetConsoleCursorPosition(hConsole, cp);
#endif

		void
			*from = memory_from.data(),
			*to = memory_to.data();

#define TEST(TEST_to, TEST_from, TEST_size, TEST_func, TEST_name, TEST_ti) { \
			std::memset(TEST_to, 0, TEST_size); \
					{ \
				Timing t(&TEST_ti); \
				TEST_func(TEST_to, TEST_from, TEST_size); \
					} \
			bool TEST_valid = true; \
			uint8_t *TEST_vfrom = (uint8_t*)TEST_from, *TEST_vto = (uint8_t*)TEST_to; \
			for (size_t TEST_p = 0; TEST_p < TEST_size; TEST_p++) { \
				TEST_valid = TEST_valid && (TEST_vfrom[TEST_p] == TEST_vto[TEST_p]); \
						} \
			if (TEST_valid) \
				TEST_ti.validCalls++; \
			formattedPrint(&TEST_ti, TEST_name); \
				}

		TEST(to, from, memorysize, memcpy_c, "C", ti_c);
		TEST(to, from, memorysize, memcpy_cpp, "C++", ti_cpp);
		for (size_t n = 2; n <= max_threads; n++) {
			std::vector<char> name(1024);
			sprintf_s(name.data(), name.size(), "C++ %u Threads", n);
			memcpy_thread_env(ti_threads_env[n]);
			TEST(to, from, memorysize, memcpy_thread, name.data(), ti_threads[n]);
		}
	}

	for (size_t t = 2; t <= max_threads; t++) {
		memcpy_thread_finalize(ti_threads_env[t]);
	}

	getchar();
	return 0;
}

