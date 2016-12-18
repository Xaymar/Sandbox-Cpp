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

#define USE_LOMONT_CONSTANT
#ifndef USE_LOMONT_CONSTANT
#define FASTINVSQRT 0x5F3759DF // Carmack
#else
#define FASTINVSQRT 0x5F375A86 // Chris Lomont
#endif

float test_invsqrt_carmack(float v) {
	union {
		float f;
		long u;
	} y = { v };
	float x2 = v * 0.5f;
	y.u = FASTINVSQRT - (y.u >> 1);
	y.f = (1.5f - (x2 * y.f * y.f));
	return y.f;
}

float test_invsqrt_carmack_2iter(float v) {
	union {
		float f;
		long u;
	} y = { v };
	float x2 = v * 0.5f;
	y.u = FASTINVSQRT - (y.u >> 1);
	y.f = (1.5f - (x2 * y.f * y.f));
	y.f = (1.5f - (x2 * y.f * y.f));
	return y.f;
}

float test_invsqrt_carmack_sse(float v) {
	const __m128i magic_constant = _mm_set_epi32(FASTINVSQRT, 0, 0, 0);
	const __m128 zero_point_five = _mm_set_ps(0.5f, 0, 0, 0);
	const __m128 one_point_five = _mm_set_ps(1.5f, 0, 0, 0);

	__m128 value = _mm_set_ps(v, 0, 0, 0); // y.f = v
	__m128 halfvalue = _mm_mul_ps(value, zero_point_five); // x2 = v * 0.5f
	__m128i ivalue = _mm_castps_si128(value); // y.u (union) y.f
	ivalue = _mm_srai_epi32(ivalue, 1); // y.u >> 1
	ivalue = _mm_sub_epi32(magic_constant, ivalue); // FASTINVSQRT - (y.u >> 1)
	value = _mm_castsi128_ps(ivalue); // y.f (union) y.u

	// y.f = 1.5f - (x2 * y.f * y.f) part
	value = _mm_mul_ps(value, value); // y.f * y.f
	value = _mm_mul_ps(value, halfvalue); // x2 * y.f * y.f
	value = _mm_sub_ps(one_point_five, value); // 1.5f - (x2 * y.f * y.f)

	float result[4];
	_mm_storeu_ps(result, value);
	return result[0];
}

void test_invsqrt_carmack_sse_quad(float v1, float v2, float v3, float v4, float* result) {
	const __m128i magic_constant = _mm_set_epi32(FASTINVSQRT, FASTINVSQRT, FASTINVSQRT, FASTINVSQRT);
	const __m128 zero_point_five = _mm_set_ps(0.5f, 0.5f, 0.5f, 0.5f);
	const __m128 one_point_five = _mm_set_ps(1.5f, 1.5f, 1.5f, 1.5f);

	__m128 value = _mm_set_ps(v1, v2, v3, v4); // y.f = v
	__m128 halfvalue = _mm_mul_ps(value, zero_point_five); // x2 = v * 0.5f
	__m128i ivalue = _mm_castps_si128(value); // y.u (union) y.f
	ivalue = _mm_srai_epi32(ivalue, 1); // y.u >> 1
	ivalue = _mm_sub_epi32(magic_constant, ivalue); // FASTINVSQRT - (y.u >> 1)
	value = _mm_castsi128_ps(ivalue); // y.f (union) y.u

	// y.f = 1.5f - (x2 * y.f * y.f) part
	value = _mm_mul_ps(value, value); // y.f * y.f
	value = _mm_mul_ps(value, halfvalue); // x2 * y.f * y.f
	value = _mm_sub_ps(one_point_five, value); // 1.5f - (x2 * y.f * y.f)

	// result
	_mm_storeu_ps(result, value);
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

typedef void(*sqrt_func_quad_t)(float, float, float, float, float*);
test_data test_sse_quad(float testValue, size_t testSize, sqrt_func_quad_t func) {
	// Version for testing SSE on four floats at once.

	float x = 0;
	size_t tmpTestSizeLoop = testSize / 4; // Divide by 4.
	size_t tmpTestSizeRem = testSize % 4; // Remaining parts.

	std::chrono::high_resolution_clock::duration t_total = std::chrono::nanoseconds(0);
	for (size_t run = 0; run < tmpTestSizeLoop; run++) {
		float y[4];
		auto t_start = std::chrono::high_resolution_clock::now();
		func(testValue, testValue, testValue, testValue, y);
		auto t_time = std::chrono::high_resolution_clock::now() - t_start;
		x += y[0] + y[1] + y[2] + y[3];
		t_total += t_time;
	}
	float par[4], res[4];
	for (size_t run = 0; run < tmpTestSizeRem; run++) {
		par[run] = testValue;
	}
	auto t_start = std::chrono::high_resolution_clock::now();
	func(par[0], par[1], par[2], par[3], res);
	auto t_time = std::chrono::high_resolution_clock::now() - t_start;
	x += res[0] + res[1] + res[2] + res[3];
	t_total += t_time;

	return std::make_tuple(t_total, testValue, x);
}

int main(int argc, const char** argv) {
	float testValue = 1234.56789f;
	size_t testSize = 10000000;

	#ifdef _WIN32
	timeBeginPeriod(1);
	#endif

	std::cout << "InvSqrt Single Test" << std::endl;
	std::cout << " - Iterations: " << testSize << std::endl;
	std::cout << " - Tested Value: " << testValue << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: InvSqrt" << std::endl;
	auto tv_invsqrt = test(testValue, testSize, test_invsqrt);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: Quake III InvSqrt" << std::endl;
	auto tv_invsqrt_carmack = test(testValue, testSize, test_invsqrt_carmack);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_carmack).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_carmack).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: SSE InvSqrt" << std::endl;
	auto tv_invsqrt_sse = test(testValue, testSize, test_invsqrt_sse);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_sse).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_sse).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: Quake III SSE InvSqrt" << std::endl;
	auto tv_invsqrt_carmack_sse = test_sse_quad(testValue, testSize, test_invsqrt_carmack_sse_quad);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_carmack_sse).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_carmack_sse).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: SSE InvSqrt (4 Ops)" << std::endl;
	auto tv_invsqrt_sse_quad = test(testValue, testSize, test_invsqrt_sse);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_sse_quad).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_sse_quad).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: Quake III SSE InvSqrt (4 Ops)" << std::endl;
	auto tv_invsqrt_carmack_sse_quad = test_sse_quad(testValue, testSize, test_invsqrt_carmack_sse_quad);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_carmack_sse_quad).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_carmack_sse_quad).count() / (double)testSize) << "ns" << std::endl;

	#ifdef _WIN32
	timeEndPeriod(1);
	#endif

	getchar();
	return 0;
}

