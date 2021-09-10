#include <inttypes.h>
#include <math.h>
#include <iostream>
#include <chrono>
#include <intrin.h>
#ifdef _WIN32
#include <windows.h>
#endif

inline uint64_t BSRFPOT(uint64_t size) {
	unsigned long index;
	_BitScanReverse64(&index, size);
	/*size &= ~(1 << index);
	if (size > 0)
		index++;*/
	return 1ull << index;
}

inline uint64_t LoopBasedFPOT(uint64_t size) {
	uint8_t shift = 0;
	while (size > 1) {
		size >>= 1;
		shift++;
	}
	return 1ull << shift;
}

inline uint64_t Log10BasedFPOT(uint64_t size) {
	return 1ull << uint64_t(log10(double(size)) / log10(2.0));
}

inline uint64_t PowLog10BasedFPOT(uint64_t size) {
	return uint64_t(pow(2, uint64_t(log10(double(size)) / log10(2.0))));
}

int main(int argc, char* argv[]) {
	const uint64_t loops = 1000000;
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

	uint64_t totalBSR, totalLoop, totalLog10, totalPow;
	uint64_t totalBSR2, totalLoop2, totalLog102, totalPow2;
	uint64_t valueBSR, valueLoop, valueLog10, valuePow;
	uint64_t cnt = 0;
	totalLoop2 = totalLog102 = totalPow2 = totalBSR2 = 0;
	for (uint64_t s = 2; s < 256000; s += s >> 1) {
		totalLoop = totalLog10 = totalPow = totalBSR = 0;
		valueBSR = valueLoop = valueLog10 = valuePow = 0;

		start = std::chrono::high_resolution_clock::now();
#pragma unroll
#pragma loop(ivdep)
		for (uint64_t n = 0; n < loops; n++) {
			valueBSR += BSRFPOT(s);
		}
		end = std::chrono::high_resolution_clock::now();
		timeTaken = end - start;
		totalBSR += timeTaken.count();

		start = std::chrono::high_resolution_clock::now();
#pragma unroll
#pragma loop(ivdep)
		for (uint64_t n = 0; n < loops; n++) {
			valueLoop += LoopBasedFPOT(s);
		}
		end = std::chrono::high_resolution_clock::now();
		timeTaken = end - start;
		totalLoop += timeTaken.count();

		start = std::chrono::high_resolution_clock::now();
#pragma unroll
#pragma loop(ivdep)
		for (uint64_t n = 0; n < loops; n++) {
			valueLog10 += Log10BasedFPOT(s);
		}
		end = std::chrono::high_resolution_clock::now();
		timeTaken = end - start;
		totalLog10 += timeTaken.count();

		start = std::chrono::high_resolution_clock::now();
#pragma unroll
#pragma loop(ivdep)
		for (uint64_t n = 0; n < loops; n++) {
			valuePow += PowLog10BasedFPOT(s);
		}
		end = std::chrono::high_resolution_clock::now();
		timeTaken = end - start;
		totalPow += timeTaken.count();

		printf("Size %ld\n", s);
		printf("BSR   | Total: %16lld | Average: %8.8lf | Value %16lld\n", totalBSR, double(totalBSR) / loops, uint64_t(double(valueBSR) / loops));
		printf("Loop  | Total: %16lld | Average: %8.8lf | Value %16lld\n", totalLoop, double(totalLoop) / loops, uint64_t(double(valueLoop) / loops));
		printf("Log10 | Total: %16lld | Average: %8.8lf | Value %16lld\n", totalLog10, double(totalLog10) / loops, uint64_t(double(valueLog10) / loops));
		printf("Pow   | Total: %16lld | Average: %8.8lf | Value %16lld\n", totalPow, double(totalPow) / loops, uint64_t(double(valuePow) / loops));
		totalBSR2 += totalBSR;
		totalLoop2 += totalLoop;
		totalLog102 += totalLog10;
		totalPow2 += totalPow;
		cnt++;
	}

	printf("Overall\n");
	printf("BSR   | Total: %16lld | Average: %8.8lf\n", totalBSR2, double(totalBSR2) / loops / cnt);
	printf("Loop  | Total: %16lld | Average: %8.8lf\n", totalLoop2, double(totalLoop2) / loops / cnt);
	printf("Log10 | Total: %16lld | Average: %8.8lf\n", totalLog102, double(totalLog102) / loops / cnt);
	printf("Pow   | Total: %16lld | Average: %8.8lf\n", totalPow2, double(totalPow2) / loops / cnt);
	std::cin.get();
	return 0;
}