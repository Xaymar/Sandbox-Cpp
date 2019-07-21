// advmemcpy.cpp : Defines the entry point for the console application.
//

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "apex_memmove.h"
#include "measurer.hpp"
#include "memcpy_adv.h"

#define MEASURE_THRESHOLD_NANOSECONDS 1
#define MEASURE_TEST_CYCLES 10000

#define SIZE(W, H, C, N) \
	{ \
		W *H *C, N \
	}

using namespace std;

std::map<size_t, std::string> test_sizes{
	SIZE(1280, 720, 4, "720 RGBA"),
	// Stop that clang-format
	SIZE(1280, 720, 3, "720 I444"),
	SIZE(1280, 720, 2, "720 I420"),
	SIZE(1920, 1080, 4, "1080 RGBA"),
	SIZE(1920, 1080, 3, "1080 I444"),
	SIZE(1920, 1080, 2, "1080 I420"),
	SIZE(2560, 1440, 4, "1440 RGBA"),
	SIZE(2560, 1440, 3, "1440 I444"),
	SIZE(2560, 1440, 2, "1440 I420"),
	SIZE(3840, 2160, 4, "2160 RGBA"),
	SIZE(3840, 2160, 3, "2160 I444"),
	SIZE(3840, 2160, 2, "2160 I420"),
};

std::map<std::string, std::function<void*(void* to, void* from, size_t size)>> functions{
	{ "memcpy", &std::memcpy },
	{ "movsb", &memcpy_movsb },
	{ "movsw", &memcpy_movsw },
	{ "movsd", &memcpy_movsd },
	{ "movsq", &memcpy_movsq },
	{ "apex_memcpy", [](void* t, void* f, size_t s) { return apex::memcpy(t, f, s); } },
	{ "advmemcpy", &memcpy_thread },
	{ "advmemcpy_apex", &memcpy_thread },
};

std::map<std::string, std::function<void()>> initializers{
	{ "advmemcpy", []() { memcpy_thread_set_memcpy(std::memcpy); } },
	{ "advmemcpy_apex", []() { memcpy_thread_set_memcpy(apex::memcpy); } },
	{ "apex_memcpy", []() { apex::memcpy(0, 0, 0); } },
};

int32_t main(int32_t argc, const char* argv[])
{
	std::vector<char> buf_from, buf_to;
	buf_from.resize(1);
	buf_to.resize(1);

	auto amcenv = memcpy_thread_initialize(std::thread::hardware_concurrency());
	memcpy_thread_env(amcenv);
	apex::memcpy(buf_to.data(), buf_from.data(), 1);

	for (auto test : test_sizes) {
		std::cout << "Testing '" << test.second << "' ( " << (test.first) << " B )..." << std::endl;

		buf_from.resize(test.first);
		buf_to.resize(test.first);

		void*  from = buf_from.data();
		void*  to   = buf_to.data();
		size_t size = test.first;

		// Name            |       KB/s | 95.0% KB/s | 99.0% KB/s | 99.9% KB/s
		// ----------------+------------+------------+------------+------------

		std::cout << "Name            | Avg.  MB/s | 95.0% MB/s | 99.0% MB/s | 99.9% MB/s " << std::endl
		          << "----------------+------------+------------+------------+------------" << std::endl;

		for (auto func : functions) {
			measurer measure;

			auto inits = initializers.find(func.first);
			if (inits != initializers.end()) {
				inits->second();
			}

			std::cout << setw(16) << setiosflags(ios::left) << func.first;
			std::cout << setw(0) << resetiosflags(ios::left) << "|" << std::flush;

			// Measure initial latency.
			std::chrono::nanoseconds threshold_val;
			{
				auto tp_here = std::chrono::high_resolution_clock::now();
				func.second(to, from, size);
				auto tp_there = std::chrono::high_resolution_clock::now();
				threshold_val = tp_there - tp_here;
			}

			if (threshold_val < std::chrono::nanoseconds(MEASURE_THRESHOLD_NANOSECONDS)) {
				// Measure the entire loop.
				auto tracker = measure.track();
				for (size_t idx = 0; idx < MEASURE_TEST_CYCLES; idx++) {
					func.second(to, from, size);
				}
			} else {
				// Measure only the call itself.
				for (size_t idx = 0; idx < MEASURE_TEST_CYCLES; idx++) {
					auto tracker = measure.track();
					func.second(to, from, size);
				}
			}

			double_t size_kb   = (static_cast<double_t>(test.first) / 1024 / 1024);
			auto     time_avg  = measure.average_duration();
			auto     time_950  = measure.percentile(0.95);
			auto     time_990  = measure.percentile(0.99);
			auto     time_999  = measure.percentile(0.999);
			double_t kbyte_avg = size_kb / (static_cast<double_t>(time_avg) / 1000000000);
			double_t kbyte_950 = size_kb / (static_cast<double_t>(time_950.count()) / 1000000000);
			double_t kbyte_990 = size_kb / (static_cast<double_t>(time_990.count()) / 1000000000);
			double_t kbyte_999 = size_kb / (static_cast<double_t>(time_999.count()) / 1000000000);

			std::cout << setw(11) << setprecision(2) << setiosflags(ios::right) << std::fixed << kbyte_avg
			          << setw(0) << resetiosflags(ios::right) << " |" << std::flush;
			std::cout << setw(11) << setprecision(2) << setiosflags(ios::right) << std::fixed << kbyte_950
			          << setw(0) << resetiosflags(ios::right) << " |" << std::flush;
			std::cout << setw(11) << setprecision(2) << setiosflags(ios::right) << std::fixed << kbyte_990
			          << setw(0) << resetiosflags(ios::right) << " |" << std::flush;
			std::cout << setw(11) << setprecision(2) << setiosflags(ios::right) << std::fixed << kbyte_999
			          << setw(0) << resetiosflags(ios::right);

			std::cout << std::defaultfloat << std::endl;
		}

		// Split results
		std::cout << std::endl << std::endl;
	}

	memcpy_thread_finalize(amcenv);

	std::cin.get();
	return 0;
}
