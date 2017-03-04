// advmemcpy.cpp : Defines the entry point for the console application.
//

#include <cstring>
#include <chrono>
#include <iostream>
#include <memory.h>
#include <random>
#include <stdint.h>
#include <vector>
#include <thread>
#include <sstream>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#define snprintf sprintf_s
#endif

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

size_t memorysize = 3840 * 2160 * 4; // A single 3840x2160 RGBA frame

void formattedPrint(TimingInfo* ti, const char* name) {
	if (ti == nullptr) {
		std::cout <<
			"| Name             | Calls    | Valid    | Average Time | Minimum Time | Maximum Time | Bandwidth    |" << std::endl;
		std::cout <<
			"+------------------+----------+----------+--------------+--------------+--------------+--------------+" << std::endl;
	} else {
		std::vector<char> buf(1024);
		snprintf(buf.data(), buf.size(),
			"| %-16s | %8lld | %8lld | %9lld ns | %9lld ns | %9lld ns | %7lld mB/s |",
			name,
			ti->callAmount,
			ti->validCalls,
			(uint64_t)ti->averageCallTime,
			ti->minimumCallTime,
			ti->maximumCallTime,
			(uint64_t)((((1000000000.0 / (double_t)ti->averageCallTime) * memorysize) / 1024) / 1024));
		std::cout << buf.data() << std::endl;
	}
}

typedef void* (__cdecl*test_t)(void*, void*, size_t);
void do_test(void* to, void* from, size_t size, test_t func, const char* name, TimingInfo& ti) {
	std::memset(to, 0, size);
	{
		Timing t(&ti);
		func(to, from, size);
	}
	if (std::memcmp(from, to, size) == 0)
		ti.validCalls++;
	formattedPrint(&ti, name);
}

int main(int argc, char** argv) {
	size_t iterations = 100;

	size_t width = 3840,
		height = 2160,
		depth = 4;
	size_t size = 0;

	for (size_t argn = 0; argn < argc; argn++) {
		std::string arg = std::string(argv[argn]);
		if ((arg == "-w") || (arg == "--width")) {
			argn++;
			if (argn >= argc)
				std::cerr << "Missing Width" << std::endl;
			std::stringstream argss(argv[argn]);
			argss >> width;
		} else if ((arg == "-h") || (arg == "--height")) {
			argn++;
			if (argn >= argc)
				std::cerr << "Missing Height" << std::endl;
			std::stringstream argss(argv[argn]);
			argss >> height;
		} else if ((arg == "-d") || (arg == "--depth")) {
			argn++;
			if (argn >= argc)
				std::cerr << "Missing Depth" << std::endl;
			std::stringstream argss(argv[argn]);
			argss >> depth;
		} else if ((arg == "-s") || (arg == "--size")) {
			argn++;
			if (argn >= argc)
				std::cerr << "Missing Size" << std::endl;
			std::stringstream argss(argv[argn]);
			width = height = depth = 0;
			argss >> size;
		}
	}
	if ((width != 0) && (height != 0) && (depth != 0)) {
		size = width * height * depth;
	}
	memorysize = size;
	std::cout << "Size: " << size << " byte (Width: " << width << ", Height: " << height << ", Depth: " << depth << ")" << std::endl;

#ifdef _WIN32
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
	for (size_t it = 0; it < iterations; it++) {
#ifdef _WIN32
		cp.X = 0; cp.Y = 2;
		SetConsoleCursorPosition(hConsole, cp);
#endif

		do_test(memory_to.data(), memory_from.data(), memorysize, (test_t)memcpy, "C", ti_c);
		do_test(memory_to.data(), memory_from.data(), memorysize, (test_t)std::memcpy, "C++", ti_cpp);
		for (size_t n = 2; n <= max_threads; n++) {
			std::vector<char> name(1024);
			sprintf_s(name.data(), name.size(), "C++ %lld Threads", n);
			memcpy_thread_env(ti_threads_env[n]);
			do_test(memory_to.data(), memory_from.data(), memorysize, memcpy_thread, name.data(), ti_threads[n]);
		}
	}

	for (size_t t = 2; t <= max_threads; t++) {
		memcpy_thread_finalize(ti_threads_env[t]);
	}

	getchar();
	return 0;
}

