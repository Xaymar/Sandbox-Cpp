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
#endif

#include <stdlib.h>
#ifdef _MSV_VER
#define aligned_alloc _aligned_malloc
#define aligned_free _aligned_free
#define snprintf sprintf_s
#else
#define aligned_free free
#define snprintf(p1, p2, ...) sprintf(p1, __VA_ARGS__)
#endif


#include "memcpy_adv.h"

#define memcpy_c memcpy
#define memcpy_cpp std::memcpy

struct TimingInfo {
	friend class Timing;
	protected:
	std::chrono::high_resolution_clock::time_point _time;
	public:
	// All Time info in nanoseconds
	uint64_t
		TimeMinimum = UINT64_MAX,
		TimeMaximum = 0,
		TimeTotal = 0,
		AmountCalled = 0,
		AmountValid = 0;
	double_t
		TimeAverage = 0.0;
};

class Timing {
	private:
	TimingInfo* ti = nullptr;

	public:
	Timing(TimingInfo* p_ti) {
		ti = p_ti;
		ti->_time = std::chrono::high_resolution_clock::now();
	}
	~Timing() {
		auto hcnow = std::chrono::high_resolution_clock::now();
		uint64_t time = (hcnow - ti->_time).count();

		ti->AmountCalled++;
		ti->TimeTotal += time;

		if (time < ti->TimeMinimum)
			ti->TimeMinimum = time;
		if (time > ti->TimeMaximum)
			ti->TimeMaximum = time;

		ti->TimeAverage = (double_t)ti->TimeTotal / (double_t)ti->AmountCalled;
	}
};

void formattedPrint(TimingInfo* ti, const char* name, size_t memsize) {
	if (ti == nullptr) {
		std::cout <<
			"| Name             | Calls    | Valid    | Average Time | Minimum Time | Maximum Time | Bandwidth            |" << std::endl;
		std::cout <<
			"+------------------+----------+----------+--------------+--------------+--------------+----------------------+" << std::endl;
	} else {
		size_t totalMemoryCopied = ti->AmountCalled * memsize;
		double_t byte_per_second = (double_t)totalMemoryCopied / ((double_t)ti->TimeTotal / 1000000000.0);

		std::vector<char> buf(1024);
		snprintf(buf.data(), buf.size(),
			"| %-16s | %8lld | %8lld | %9lld ns | %9lld ns | %9lld ns | %16lld B/s |",
			name,
			ti->AmountCalled,
			ti->AmountValid,
			(uint64_t)ti->TimeAverage,
			ti->TimeMinimum,
			ti->TimeMaximum,
			(uint64_t)byte_per_second);
		std::cout << buf.data() << std::endl;
	}
}

typedef void* (__cdecl*test_t)(void*, void*, size_t);
void do_test(void* to, void* from, size_t size, test_t func, const char* name, TimingInfo& ti, size_t iter) {
	unsigned char* pto = (unsigned char*)to;
	while (iter > 0) {
		iter--;
		pto++;

		std::memset(pto, 0, size);
		{
			Timing t(&ti);
			func(pto, from, size);
		}
		if (std::memcmp(from, pto, size) == 0)
			ti.AmountValid++;
	}
	formattedPrint(&ti, name, size);
}
void do_print(const char* name, TimingInfo& ti, size_t size) {
	formattedPrint(&ti, name, size);
}

int main(int argc, char** argv) {
	size_t iterations = 100;
	size_t width = 3840, height = 2160, depth = 4;
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
		} else if ((arg == "-i") || (arg == "--iterations")) {
			argn++;
			if (argn >= argc)
				std::cerr << "Missing Iterations" << std::endl;
			std::stringstream argss(argv[argn]);
			argss >> iterations;
		}
	}
	//if ((width != 0) && (height != 0) && (depth != 0)) {
	if (size == 0)
		size = width * height * depth;
	std::cout << "Size: " << size << " byte (Width: " << width << ", Height: " << height << ", Depth: " << depth << ")" << std::endl;
	std::cout << "Iterations: " << iterations << std::endl;

	#ifdef _WIN32
	timeBeginPeriod(1);
	//SetThreadAffinityMask(GetCurrentThread(), 0x1);
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	#else
	#endif

	std::cout << "Initializing..." << std::endl;
	srand((unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count());
	char* memory_from = new char[size + iterations];
	char* memory_to = new char[size + iterations];
	for (size_t p = 0; p < size + iterations; p++) {
		// Ensure memory is randomized.
		memory_from[p] = (char)rand();
	}
	std::memset(memory_to, 0, size + iterations);

	const size_t max_threads = 8;
	void* ti_threads_env[max_threads + 1];
	for (size_t t = 2; t <= max_threads; t++) {
		ti_threads_env[t] = memcpy_thread_initialize(t);
	}

	formattedPrint(nullptr, nullptr, 0);

	//for (size_t n = 16384; n < size; n <<= 1) {
	//	std::cout << "Block Size: " << n << std::endl;
	size_t n = size;
		TimingInfo
			ti_c,
			ti_cpp,
			ti_movsb;
		//TimingInfo ti_threads[max_threads + 1];
		do_test(memory_to, memory_from, n, (test_t)memcpy, "C", ti_c, iterations);
		do_test(memory_to, memory_from, n, (test_t)std::memcpy, "C++", ti_cpp, iterations);
		do_test(memory_to, memory_from, n, (test_t)memcpy_movsb, "MOVSB", ti_movsb, iterations);
		for (size_t n2 = 2; n2 <= max_threads; n2++) {
			TimingInfo ti_thread;
			std::vector<char> name(1024);
			snprintf(name.data(), name.size(), "MT: %lld Threads", n2);
			memcpy_thread_env(ti_threads_env[n2]);
			do_test(memory_to, memory_from, n, memcpy_thread, name.data(), ti_thread, iterations);
		}
	//}

	for (size_t t = 2; t <= max_threads; t++) {
		memcpy_thread_finalize(ti_threads_env[t]);
	}

	delete[] memory_to;
	delete[] memory_from;

	#ifdef _WIN32
	timeEndPeriod(1);
	#endif

	getchar();
	return 0;
	}

