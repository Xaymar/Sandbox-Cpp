#include <atomic>
#include <chrono>
#include <cinttypes>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#define XMR_UTILITY_PROFILER_ENABLE_FORCEINLINE
#include <xmr/utility/profiler/clock/tsc.hpp>
#include <xmr/utility/profiler/profiler.hpp>

/* Measure Core To Core Latency

Modern CPUs have variable latency between cores, which results in poor performance
when you suddenly encounter higher latency going across core boundaries.


As an example in order to measure C2C latency between Core 1 and Core 2:
- Spin up two threads, Thread A and B.
- A & B assign their own priority to real-time.
- A should set its own affinity to Core 1.
- B should set its own affinity to Core 2.
- A spin locks and waits for B to be ready.
- B sets a signal that it is ready and spin locks on an atomic variable.
- A records time, signals B to record time.
- B records time.
- Repeat for N iterations and all cores.



*/

#define ITERATIONS 100000

void thread_affinity(uint16_t processor, uint8_t thread_index)
{
#ifdef WIN32
	HANDLE         thread = GetCurrentThread();
	GROUP_AFFINITY gaff   = {0};
	gaff.Group            = processor;
	gaff.Mask             = 0b1ull << thread_index;

	PROCESSOR_NUMBER pn = {0};
	pn.Group            = gaff.Group;
	pn.Number           = thread_index;

	SetThreadIdealProcessorEx(thread, &pn, nullptr);
	SetThreadGroupAffinity(thread, &gaff, nullptr);
#else
	pthread_set_Affinity_np()
#endif
}

void thread_priority_rt()
{
#ifdef WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#else
#endif
}

struct thread_read_data {
	volatile uint32_t            id;
	volatile std::atomic<size_t> ready;

	// Profiler storage
	std::shared_ptr<xmr::utility::profiler::profiler> profiler;
};

struct thread_write_data {
	volatile uint32_t            id;
	volatile std::atomic<size_t> ready;
	volatile std::atomic<bool>   data;
	volatile uint64_t            time;
};

void thread_read_main(thread_read_data* td, thread_write_data* twd)
{
	thread_affinity(0, td->id);
	thread_priority_rt();

	volatile std::atomic<size_t>& read_ready  = td->ready;
	volatile std::atomic<size_t>& write_ready = twd->ready;
	volatile std::atomic<bool>&   data        = twd->data;

	read_ready = 0;
	data       = false;
	for (volatile size_t idx = 1; idx <= ITERATIONS; idx++) {
		// Wait for write thread to be ready.
		while (write_ready != idx) { // no-op
		}
		// Signal write thread that read thread is ready.
		read_ready = idx;
		// Wait until we are signalled
		while (!data) { // no-op
		}
		// Record time and store.
		uint64_t time = xmr::utility::profiler::clock::tsc::now();
		td->profiler->track(time, twd->time);
		// Reset data
		data = false;
	}
}

void thread_write_main(thread_write_data* td, thread_read_data* trd)
{
	thread_affinity(0, td->id);
	thread_priority_rt();

	volatile std::atomic<size_t>& read_ready  = trd->ready;
	volatile std::atomic<size_t>& write_ready = td->ready;
	volatile std::atomic<bool>&   data        = td->data;

	write_ready = 0;
	for (volatile size_t idx = 1; idx <= ITERATIONS; idx++) {
		// Signal read thread to be ready.
		write_ready = idx;
		// Wait for read thread to be ready.
		while (read_ready != idx) { // no-op
		}
		// Record time and signal read thread.
		td->time = xmr::utility::profiler::clock::tsc::now();
		data     = true;
		// Wait until read thread resets data.
		while (data) { // no-op
		}
	}
}

std::int32_t main(std::int32_t argc, const char* argv[])
{
	std::map<std::pair<uint32_t, uint32_t>, std::shared_ptr<xmr::utility::profiler::profiler>> profilers;

	uint32_t max_core_id = std::thread::hardware_concurrency();
	for (uint32_t idx = 0; idx < max_core_id; idx++) {
		printf("%3" PRIu32 " |", idx);
		for (uint32_t jdx = 0; jdx < max_core_id; jdx++) {
			std::pair<uint32_t, uint32_t> key{idx, jdx};

			// Skip identical cores, can't measure latency to self.
			if (idx == jdx)
				continue;

			// Create and initialize structures.
			thread_read_data  trd;
			thread_write_data twd;
			trd.id       = idx;
			trd.profiler = std::make_shared<xmr::utility::profiler::profiler>();
			twd.id       = jdx;

			// Spawn both threads.
			std::thread a(thread_read_main, &trd, &twd);
			std::thread b(thread_write_main, &twd, &trd);

			// Block by joining back together with the threads.
			if (a.joinable())
				a.join();
			if (b.joinable())
				b.join();

			// Insert measurements
			profilers.emplace(key, trd.profiler);

			printf("%6.1f ns |", xmr::utility::profiler::clock::tsc::to_nanoseconds(trd.profiler->average_time()));
		}
		printf("\n");
	}

	// Write results to file.
	std::ofstream file("results.csv", std::ios_base::out | std::ios_base::trunc);
	{ // Average
		file << "c2c" << ",";
		for (size_t n = 0; n < max_core_id; n++) {
			file << n << ",";
		}
		file << std::endl;
		for (size_t n = 0; n < max_core_id; n++) {
			file << n << ",";
			for (size_t m = 0; m < max_core_id; m++) {
				std::pair<uint32_t, uint32_t> key{n, m};
				if (n == m) {
					file << "x"
						 << ",";
					continue;
				}

				auto value = profilers.find(key);
				if (value != profilers.end()) {
					file << xmr::utility::profiler::clock::tsc::to_nanoseconds(value->second->average_time()) << ",";
				}
			}
			file << std::endl;
		}
		file << std::endl;
	}
	{ // 99.90ile
		file << "c2c" << ",";
		for (size_t n = 0; n < max_core_id; n++) {
			file << n << ",";
		}
		file << std::endl;
		for (size_t n = 0; n < max_core_id; n++) {
			file << n << ",";
			for (size_t m = 0; m < max_core_id; m++) {
				std::pair<uint32_t, uint32_t> key{n, m};
				if (n == m) {
					file << "x"
						 << ",";
					continue;
				}

				auto value = profilers.find(key);
				if (value != profilers.end()) {
					file << xmr::utility::profiler::clock::tsc::to_nanoseconds(value->second->percentile_events(0.999))
						 << ",";
				}
			}
			file << std::endl;
		}
		file << std::endl;
	}
	{ // 99.00ile
		file << "c2c" << ",";
		for (size_t n = 0; n < max_core_id; n++) {
			file << n << ",";
		}
		file << std::endl;
		for (size_t n = 0; n < max_core_id; n++) {
			file << n << ",";
			for (size_t m = 0; m < max_core_id; m++) {
				std::pair<uint32_t, uint32_t> key{n, m};
				if (n == m) {
					file << "x"
						 << ",";
					continue;
				}

				auto value = profilers.find(key);
				if (value != profilers.end()) {
					file << xmr::utility::profiler::clock::tsc::to_nanoseconds(value->second->percentile_events(0.99))
						 << ",";
				}
			}
			file << std::endl;
		}
		file << std::endl;
	}
	file.close();

	// Wait for user to hit enter.
	std::cin.get();

	return 0;
}
