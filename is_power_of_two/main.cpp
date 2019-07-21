#include <inttypes.h>
#include <math.h>
#include <iostream>
#include <chrono>
#include <intrin.h>
#ifdef _WIN32
#include <windows.h>
#endif

inline bool ispot_bitscan(uint64_t size) {
	unsigned long index, index2;
	_BitScanReverse64(&index, size);
	_BitScanForward64(&index2, size);

	return index == index2;
}

inline bool ispot_loop(uint64_t size) {
	//bool have_bit = false;
	//for (size_t index = 0; index < 64; index++) {
	//	bool cur = (size & (1ull << index)) != 0;
	//	if (have_bit && cur)
	//		return false;
	//	if (cur) 
	//		have_bit = true;
	//}
	//return true; 
	bool have_bit = false;
	for (size_t index = 63; index >= 0; index--) {
		bool cur = (size & (1ull << index)) != 0;
		if (cur) {
			if (have_bit)
				return false;
			have_bit = true;
		}
	}
	return true;
}

inline bool ispot_log10(uint64_t size) {
	return (1ull << uint64_t(log10(double(size)) / log10(2.0))) == size;
}


int main(int argc, char* argv[]) {
	const uint64_t loops = 10000000;
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point end;
	std::chrono::high_resolution_clock::duration timeTaken;

#ifdef _WIN32
	timeBeginPeriod(1);
	//SetThreadAffinityMask(GetCurrentThread(), 0x1);
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#else
#endif

	uint64_t totalBitScan, totalLoop, totalLog10, totalPow;
	uint64_t overallBitScan, overallLoop, overallLog10, totalPow2;
	uint64_t valueBitScan, valueLoop, valueLog10, valuePow;
	uint64_t cnt = 0;
	overallLoop = overallLog10 = totalPow2 = overallBitScan = 0;
	for (uint64_t s = 2; s < 16384; s += 16) {
		totalLoop = totalLog10 = totalPow = totalBitScan = 0;
		valueBitScan = valueLoop = valueLog10 = valuePow = 0;

		start = std::chrono::high_resolution_clock::now();
#pragma unroll
#pragma loop(ivdep)
		for (uint64_t n = 0; n < loops; n++) {
			valueBitScan += ispot_bitscan(s) ? 1 : 0;
		}
		end = std::chrono::high_resolution_clock::now();
		timeTaken = end - start;
		totalBitScan += timeTaken.count();

		start = std::chrono::high_resolution_clock::now();
#pragma unroll
#pragma loop(ivdep)
		for (uint64_t n = 0; n < loops; n++) {
			valueLoop += ispot_loop(s) ? 1 : 0;
		}
		end = std::chrono::high_resolution_clock::now();
		timeTaken = end - start;
		totalLoop += timeTaken.count();

		start = std::chrono::high_resolution_clock::now();
#pragma unroll
#pragma loop(ivdep)
		for (uint64_t n = 0; n < loops; n++) {
			valueLog10 += ispot_log10(s) ? 1 : 0;
		}
		end = std::chrono::high_resolution_clock::now();
		timeTaken = end - start;
		totalLog10 += timeTaken.count();



		//printf("Size %ld\n", s);
		//printf("BScan | Total: %16lld | Average: %8.8lf | Value %16lld\n", totalBitScan, double(totalBitScan) / loops, uint64_t(double(valueBitScan) / loops));
		//printf("Loop  | Total: %16lld | Average: %8.8lf | Value %16lld\n", totalLoop, double(totalLoop) / loops, uint64_t(double(valueLoop) / loops));
		//printf("Log10 | Total: %16lld | Average: %8.8lf | Value %16lld\n", totalLog10, double(totalLog10) / loops, uint64_t(double(valueLog10) / loops));
		//printf("Pow   | Total: %16lld | Average: %8.8lf | Value %16lld\n", totalPow, double(totalPow) / loops, uint64_t(double(valuePow) / loops));
		overallBitScan += totalBitScan;
		overallLoop += totalLoop;
		overallLog10 += totalLog10;
		totalPow2 += totalPow;
		cnt++;
	}

	printf("Overall\n");
	printf("BScan | Total: %16lld | Average: %8.8lf\n", overallBitScan, double(overallBitScan) / loops / cnt);
	printf("Loop  | Total: %16lld | Average: %8.8lf\n", overallLoop, double(overallLoop) / loops / cnt);
	printf("Log10 | Total: %16lld | Average: %8.8lf\n", overallLog10, double(overallLog10) / loops / cnt);
	printf("Pow   | Total: %16lld | Average: %8.8lf\n", totalPow2, double(totalPow2) / loops / cnt);
	std::cin.get();
	return 0;
}