// FloatMathSpeedTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <chrono>
#include <vector>
#include <tuple>
#include <xmmintrin.h>
#include <iostream>
#include <thread>
#include <random>
#ifdef _WIN32
#include <windows.h>
#endif

float test_invsqrt(float v) {
	return 1.0f / sqrtf(v);
}

float test_invsqrt_sse(float v) {
	__m128 mv = _mm_set_ps(v, 0, 0, 0);
	const __m128 dv = _mm_set_ps(1.0f, 0, 0, 0);
	auto sv = _mm_sqrt_ps(mv);
	sv = _mm_div_ps(dv, sv);
	float result[4];
	_mm_storeu_ps(result, sv);
	return result[0];
}

void test_invsqrt_sse_quad(float v1, float v2, float v3, float v4, float* result) {
	const __m128 dv = _mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f);
	__m128 mv = _mm_set_ps(v1, v2, v3, v4);

	auto sv = _mm_sqrt_ps(mv);
	sv = _mm_div_ps(dv, sv);
	_mm_storeu_ps(result, sv);
}

float test_invsqrt_carmack(float v) {
	union {
		float f;
		long u;
	} y = { v };
	float x2 = v * 0.5f;
	y.u = 0x5f3759df - (y.u >> 1);
	y.f = (1.5f - (x2 * y.f * y.f));
	return y.f;
}

float test_invsqrt_carmack_2iter(float v) {
	union {
		float f;
		long u;
	} y = { v };
	float x2 = v * 0.5f;
	y.u = 0x5F3759DF - (y.u >> 1);
	y.f = (1.5f - (x2 * y.f * y.f));
	y.f = (1.5f - (x2 * y.f * y.f));
	return y.f;
}

float test_invsqrt_lomont(float v) {
	union {
		float f;
		long u;
	} y = { v };
	float x2 = v * 0.5f;
	y.u = 0x5F375A86 - (y.u >> 1);
	y.f = (1.5f - (x2 * y.f * y.f));
	return y.f;
}

float test_invsqrt_lomont_2iter(float v) {
	union {
		float f;
		long u;
	} y = { v };
	float x2 = v * 0.5f;
	y.u = 0x5F375A86 - (y.u >> 1);
	y.f = (1.5f - (x2 * y.f * y.f));
	y.f = (1.5f - (x2 * y.f * y.f));
	return y.f;
}

typedef std::tuple<std::chrono::high_resolution_clock::duration, float, float> test_data;
typedef float(*sqrt_func_t)(float);
test_data test(float testValue, size_t testSize, sqrt_func_t func) {
	float x = 0;

	std::chrono::high_resolution_clock::duration t_total = std::chrono::nanoseconds(0);
	for (size_t run = 0; run < testSize; run++) {
		auto t_start = std::chrono::high_resolution_clock::now();
		float y = func(testValue);
		auto t_time = std::chrono::high_resolution_clock::now() - t_start;
		x += y;
		t_total += t_time;
	}

	return std::make_tuple(t_total, testValue, x);
}

test_data test_sse_quad(float testValue, size_t testSize) {
	// Version for testing SSE on four floats at once.

	float x = 0;
	size_t tmpTestSizeLoop = testSize / 4; // Divide by 4.
	size_t tmpTestSizeRem = testSize % 4; // Remaining parts.

	std::chrono::high_resolution_clock::duration t_total = std::chrono::nanoseconds(0);
	for (size_t run = 0; run < tmpTestSizeLoop; run++) {
		float y[4];
		auto t_start = std::chrono::high_resolution_clock::now();
		test_invsqrt_sse_quad(testValue, testValue, testValue, testValue, y);
		auto t_time = std::chrono::high_resolution_clock::now() - t_start;
		x += y[0] + y[1] + y[2] + y[3];
		t_total += t_time;
	}
	if (tmpTestSizeRem >= 1) {
		auto t_start = std::chrono::high_resolution_clock::now();
		float y = test_invsqrt_sse(testValue);
		auto t_time = std::chrono::high_resolution_clock::now() - t_start;
		x += y;
		t_total += t_time;
	}
	if (tmpTestSizeRem >= 2) {
		auto t_start = std::chrono::high_resolution_clock::now();
		float y = test_invsqrt_sse(testValue);
		auto t_time = std::chrono::high_resolution_clock::now() - t_start;
		x += y;
		t_total += t_time;
	}
	if (tmpTestSizeRem >= 3) {
		auto t_start = std::chrono::high_resolution_clock::now();
		float y = test_invsqrt_sse(testValue);
		auto t_time = std::chrono::high_resolution_clock::now() - t_start;
		x += y;
		t_total += t_time;
	}

	return std::make_tuple(t_total, testValue, x);
}

void printlog(const char* format, ...) {

}

int main(int argc, const char** argv) {
	size_t testSize = 10000000;
	std::random_device rd;
	std::mt19937 e2(rd());
	e2.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	float testValue = 1.23456789f;//((e2() + INT_MIN) / 16777216.0);

	#ifdef _WIN32
	timeBeginPeriod(1);
	#endif
	
	std::cout << "Testing InvSqrt Functions..." << std::endl;
	std::cout << "Parameters: Value (" << testValue << "), Iterations (" << testSize << ")" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: InvSqrt..." << std::endl;
	auto tv_invsqrt = test(testValue, testSize, test_invsqrt);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: InvSqrt SSE..." << std::endl;
	auto tv_invsqrt_sse = test(testValue, testSize, test_invsqrt_sse);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_sse).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_sse).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: InvSqrt SSE Quad..." << std::endl;
	auto tv_invsqrt_sse_quad = test_sse_quad(testValue, testSize);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_sse_quad).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_sse_quad).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: InvSqrt Carmack..." << std::endl;
	auto tv_invsqrt_carmack = test(testValue, testSize, test_invsqrt_carmack);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_carmack).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_carmack).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: InvSqrt Carmack 2-Iterations..." << std::endl;
	auto tv_invsqrt_carmack2 = test(testValue, testSize, test_invsqrt_carmack_2iter);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_carmack2).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_carmack2).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: InvSqrt Lomont..." << std::endl;
	auto tv_invsqrt_lomont = test(testValue, testSize, test_invsqrt_lomont);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_lomont).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_lomont).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: InvSqrt Lomont 2-Iterations..." << std::endl;
	auto tv_invsqrt_lomont2 = test(testValue, testSize, test_invsqrt_lomont_2iter);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_lomont2).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_lomont2).count() / (double)testSize) << "ns" << std::endl;

	#ifdef _WIN32
	timeEndPeriod(1);
	#endif

	getchar();
	return 0;
}

