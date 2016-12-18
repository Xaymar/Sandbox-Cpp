// InvSqrt_Double.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <random>

#include <chrono>
#include <thread>

#include <vector>
#include <tuple>

//#include <emmintrin.h>
//#include <ymmintrin.h>
#include <xmmintrin.h>
#include <intrin.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Math
double invsqrt(double v) {
	return 1.0 / sqrt(v);
}

// SSE Math
double invsqrt_sse(double v) {
	const __m128d divisor = _mm_set_pd(1.0, 0);
	__m128d value = _mm_set_pd(v, 0);
	value = _mm_sqrt_pd(value);
	value = _mm_div_pd(divisor, value);
	double res;
	_mm_storel_pd(&res, value);
	return res;
}
void invsqrt_sse2(double* v) {
	const __m128d divisor = _mm_set_pd(1.0, 1.0);
	__m128d value = _mm_set_pd(v[0], v[1]);
	value = _mm_sqrt_pd(value);
	value = _mm_div_pd(divisor, value);
	_mm_storeu_pd(v, value);
}

// Quake III
#define Q3_INVSQRT_CONSTANT 0x5fe6eb50c7b537a9ll
double invsqrt_q3(double v) {
	union {
		double f;
		long long u;
	} y = { v };
	double x2 = y.f * 0.5;
	y.u = Q3_INVSQRT_CONSTANT - (y.u >> 1);
	y.f = 1.5 * (x2 * y.f * y.f);
	return y.f;
}

// Quake III SSE
double invsqrt_q3_sse(double v) {
	const __m128i magic_constant = _mm_set_epi64x(Q3_INVSQRT_CONSTANT, 0);
	const __m128d zero_point_five = _mm_set_pd(0.5, 0);
	const __m128d one_point_five = _mm_set_pd(1.5, 0);
	
	__m128d value = _mm_load1_pd(&v);//(v, 0);
	__m128d halfvalue = _mm_mul_pd(value, one_point_five);
	__m128i ivalue = _mm_castpd_si128(value);
	//__m128i ivalue2 = _mm_srli_epi64(ivalue, 1);
	ivalue.m128i_i64[0] = ivalue.m128i_i64[0] >> 1; // No Arithmethic shift right available for 64-bit int.
	_mm_sub_epi64(magic_constant, ivalue);
	value = _mm_castsi128_pd(ivalue);

	// y.f = 1.5f - (x2 * y.f * y.f) part
	value = _mm_mul_pd(value, value); // y.f * y.f
	value = _mm_mul_pd(value, halfvalue); // x2 * y.f * y.f
	value = _mm_sub_pd(one_point_five, value); // 1.5f - (x2 * y.f * y.f)

	double res;
	_mm_storel_pd(&res, value);
	return res;
}
void invsqrt_q3_sse2(double* v) {
	const __m128i magic_constant = _mm_set_epi64x(Q3_INVSQRT_CONSTANT, Q3_INVSQRT_CONSTANT);
	const __m128d zero_point_five = _mm_set_pd(0.5, 0.5);
	const __m128d one_point_five = _mm_set_pd(1.5, 1.5);

	__m128d value = _mm_set_pd(v[0], v[1]);
	__m128d halfvalue = _mm_mul_pd(value, one_point_five);
	__m128i ivalue = _mm_castpd_si128(value);
	//__m128i ivalue2 = _mm_srli_epi64(ivalue, 1);
	ivalue.m128i_i64[0] = ivalue.m128i_i64[0] >> 1; // No Arithmetic shift right available for 64-bit int.
	ivalue.m128i_i64[1] = ivalue.m128i_i64[1] >> 1; // No Arithmetic shift right available for 64-bit int.
	_mm_sub_epi64(magic_constant, ivalue);
	value = _mm_castsi128_pd(ivalue);

	// y.f = 1.5f - (x2 * y.f * y.f) part
	value = _mm_mul_pd(value, value); // y.f * y.f
	value = _mm_mul_pd(value, halfvalue); // x2 * y.f * y.f
	value = _mm_sub_pd(one_point_five, value); // 1.5f - (x2 * y.f * y.f)

	_mm_storeu_pd(v, value);
}

typedef std::tuple<std::chrono::high_resolution_clock::duration, double, double> test_data;
typedef double(*sqrt_func_t)(double);
test_data test(double testValue, size_t testSize, sqrt_func_t func) {
	double x = 0;

	std::chrono::high_resolution_clock::duration t_total = std::chrono::nanoseconds(0);
	for (size_t run = 0; run < testSize; run++) {
		auto t_start = std::chrono::high_resolution_clock::now();
		double y = func(testValue);
		auto t_time = std::chrono::high_resolution_clock::now() - t_start;
		x += y;
		t_total += t_time;
	}

	return std::make_tuple(t_total, testValue, x);
}

typedef void(*sqrt_func_sse_t)(double*);
test_data test_sse(double testValue, size_t testSize, sqrt_func_sse_t func) {
	// Version for testing SSE on two doubles at once.

	double x = 0;
	size_t tmpTestSizeLoop = testSize / 2; // Divide by 4.
	size_t tmpTestSizeRem = testSize % 2; // Remaining parts.

	std::chrono::high_resolution_clock::duration t_total = std::chrono::nanoseconds(0);
	for (size_t run = 0; run < tmpTestSizeLoop; run++) {
		double value[2];
		value[0] = testValue;
		value[1] = testValue;

		auto t_start = std::chrono::high_resolution_clock::now();
		func(value);
		auto t_time = std::chrono::high_resolution_clock::now() - t_start;

		x += value[0] + value[1];
		t_total += t_time;
	}

	double value[2];
	value[0] = 0;
	value[1] = 0;
	for (size_t run = 0; run < tmpTestSizeRem; run++) {
		value[run] = testValue;
	}
	auto t_start = std::chrono::high_resolution_clock::now();
	func(value);
	auto t_time = std::chrono::high_resolution_clock::now() - t_start;
	x += value[0] + value[1];
	t_total += t_time;

	return std::make_tuple(t_total, testValue, x);
}

int main() {
	double testValue = 1234.56789;
	size_t testSize = 10000000;

	#ifdef _WIN32
	timeBeginPeriod(1);
	#endif

	std::cout << "InvSqrt Double Test" << std::endl;
	std::cout << " - Iterations: " << testSize << std::endl;
	std::cout << " - Tested Value: " << testValue << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: InvSqrt" << std::endl;
	auto tv_invsqrt = test(testValue, testSize, invsqrt);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: Quake III InvSqrt" << std::endl;
	auto tv_invsqrt_q3 = test(testValue, testSize, invsqrt_q3);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_q3).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_q3).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: SSE InvSqrt" << std::endl;
	auto tv_invsqrt_sse = test(testValue, testSize, invsqrt_sse);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_sse).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_sse).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: Quake III SSE InvSqrt" << std::endl;
	auto tv_invsqrt_q3_sse = test(testValue, testSize, invsqrt_q3_sse);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_q3_sse).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_q3_sse).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: SSE InvSqrt (2 Ops)" << std::endl;
	auto tv_invsqrt_sse2 = test_sse(testValue, testSize, invsqrt_sse2);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_sse2).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_sse2).count() / (double)testSize) << "ns" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << ":: Quake III SSE InvSqrt (2 Ops)" << std::endl;
	auto tv_invsqrt_q3_sse2 = test_sse(testValue, testSize, invsqrt_q3_sse2);
	std::cout << "  Total:  " << (uint64_t)(std::get<0>(tv_invsqrt_q3_sse2).count()) << "ns" << std::endl;
	std::cout << "  Single: " << (double)(std::get<0>(tv_invsqrt_q3_sse2).count() / (double)testSize) << "ns" << std::endl;


	#ifdef _WIN32
	timeEndPeriod(1);
	#endif

	getchar();
	return 0;
}

