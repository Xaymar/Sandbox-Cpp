#include <chrono>
#include <cinttypes>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>
#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#endif

// Profiler
#define XMR_UTILITY_PROFILER_ENABLE_FORCEINLINE
#include <xmr/utility/profiler/clock/hpc.hpp>
#include <xmr/utility/profiler/clock/tsc.hpp>
#include <xmr/utility/profiler/profiler.hpp>

#define ITERATIONS 1000
#define INNER_ITERATIONS 10000

#define REPEAT_10(A) \
	A;               \
	A;               \
	A;               \
	A;               \
	A;               \
	A;               \
	A;               \
	A;               \
	A;               \
	A;
#define REPEAT_100(A) \
	REPEAT_10(A);     \
	REPEAT_10(A);     \
	REPEAT_10(A);     \
	REPEAT_10(A);     \
	REPEAT_10(A);     \
	REPEAT_10(A);     \
	REPEAT_10(A);     \
	REPEAT_10(A);     \
	REPEAT_10(A);     \
	REPEAT_10(A);
#define REPEAT_1000(A) \
	REPEAT_100(A);     \
	REPEAT_100(A);     \
	REPEAT_100(A);     \
	REPEAT_100(A);     \
	REPEAT_100(A);     \
	REPEAT_100(A);     \
	REPEAT_100(A);     \
	REPEAT_100(A);     \
	REPEAT_100(A);     \
	REPEAT_100(A);
#define REPEAT_10000(A) \
	REPEAT_1000(A);     \
	REPEAT_1000(A);     \
	REPEAT_1000(A);     \
	REPEAT_1000(A);     \
	REPEAT_1000(A);     \
	REPEAT_1000(A);     \
	REPEAT_1000(A);     \
	REPEAT_1000(A);     \
	REPEAT_1000(A);     \
	REPEAT_1000(A);

template<typename _Ty1>
bool is_equal(const _Ty1 a, const _Ty1 b, const _Ty1 edge = 0.000001)
{
	// NaN values are always the same, no matter if + or -.
	if (isnan(a) && isnan(b))
		return true;

	// Infinite values are also the same, if their sign matches.
	if (isinf(a) && isinf(b) && (signbit(a) == signbit(b)))
		return true;

	// Actually test the values now.
	const _Ty1 v1 = abs(a - b);
	const _Ty1 v2 = abs(b - a);
	if ((v1 < edge) && (v2 < edge))
		return true;

	return false;
}

template<typename _Ty1>
std::shared_ptr<xmr::utility::profiler::profiler> benchmark_addsub()
{
	std::shared_ptr<xmr::utility::profiler::profiler> profile = std::make_shared<xmr::utility::profiler::profiler>();

	for (volatile size_t n = 0; n < ITERATIONS; n++) {
		const _Ty1    a = 1.0;
		const _Ty1    b = 2.0;
		const _Ty1    c = a + b * INNER_ITERATIONS - a * INNER_ITERATIONS;
		volatile _Ty1 v = 0;

		// Overhead:
		// - MSVC: 2x movss
		auto t0 = xmr::utility::profiler::clock::tsc::now();
		REPEAT_10000(v = (v + b) - a;);
		auto t1 = xmr::utility::profiler::clock::tsc::now();
		profile->track(t1, t0);
	}

	return profile;
}

template<typename _Ty1>
std::shared_ptr<xmr::utility::profiler::profiler> benchmark_muladd()
{
	std::shared_ptr<xmr::utility::profiler::profiler> profile = std::make_shared<xmr::utility::profiler::profiler>();

	for (volatile size_t n = 0; n < ITERATIONS; n++) {
		const _Ty1    a = 1.0;
		const _Ty1    b = 2.0;
		const _Ty1    c = a + b * INNER_ITERATIONS - a * INNER_ITERATIONS;
		volatile _Ty1 v = a;

		// Overhead:
		// - MSVC: 2x movss
		auto t0 = xmr::utility::profiler::clock::tsc::now();
		REPEAT_10000(v = a + (v * b););
		auto t1 = xmr::utility::profiler::clock::tsc::now();
		profile->track(t1, t0);
	}

	return profile;
}

std::int32_t main(std::int32_t argc, const char* argv[])
{
#ifdef WIN32
	SetThreadAffinityMask(GetCurrentThread(), 0b1);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	SetThreadIdealProcessor(GetCurrentThread(), 0b0);
	SetThreadPriorityBoost(GetCurrentThread(), TRUE);
	SetProcessPriorityBoost(GetCurrentProcess(), TRUE);
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
#endif

	{
		printf("Testing with %" PRIu64 "*%" PRIu64 " iterations...\n", uint64_t(ITERATIONS),
			   uint64_t(INNER_ITERATIONS));
		printf("Test      |  99.99%%  |  99.90%%  |  99.00%%  |  95.00%%  | Average  | Total    \n");
		printf("----------+----------+----------+----------+----------+----------+----------\n");
	}

	{     // Add-Sub
		{ // FP32
			auto  p = benchmark_addsub<float>();
			printf("%-10s|%8.2fns|%8.2fns|%8.2fns|%8.2fns|%8.2fns|%8.2fms\n", "F32 +-",
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9999)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9990)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9900)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9500)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->average_time()) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_milliseconds(p->total_time()));
		}
		{ // FP64
			auto   p = benchmark_addsub<double>();
			printf("%-10s|%8.2fns|%8.2fns|%8.2fns|%8.2fns|%8.2fns|%8.2fms\n", "F64 +-",
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9999)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9990)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9900)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9500)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->average_time()) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_milliseconds(p->total_time()));
		}
	}
	{     // FMA
		{ // FP32
			auto p = benchmark_muladd<float>();
			printf("%-10s|%8.2fns|%8.2fns|%8.2fns|%8.2fns|%8.2fns|%8.2fms\n", "F32 FMA",
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9999)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9990)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9900)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9500)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->average_time()) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_milliseconds(p->total_time()));
		}
		{ // FP64
			auto p = benchmark_muladd<double>();
			printf("%-10s|%8.2fns|%8.2fns|%8.2fns|%8.2fns|%8.2fns|%8.2fms\n", "F64 FMA",
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9999)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9990)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9900)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->percentile_events(0.9500)) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_nanoseconds(p->average_time()) / INNER_ITERATIONS,
				   xmr::utility::profiler::clock::tsc::to_milliseconds(p->total_time()));
		}
	}

	std::cin.get();
	return 0;
}
