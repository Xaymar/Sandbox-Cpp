// InvSqrt_Double.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <random>

#include <chrono>
#include <thread>
#include <fstream>

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
void invsqrt2(double* v) {
	v[0] = invsqrt(v[0]);
	v[1] = invsqrt(v[1]);
}
void invsqrt4(double* v) {
	invsqrt2(v);
	invsqrt2(v + 2);
}
void invsqrt8(double* v) {
	invsqrt4(v);
	invsqrt4(v + 4);
}
void invsqrt16(double* v) {
	invsqrt8(v);
	invsqrt8(v + 8);
}
void invsqrt32(double* v) {
	invsqrt16(v);
	invsqrt16(v + 16);
}
void invsqrt64(double* v) {
	invsqrt32(v);
	invsqrt32(v + 32);
}
void invsqrt128(double* v) {
	invsqrt64(v);
	invsqrt64(v + 64);
}
void invsqrt256(double* v) {
	invsqrt128(v);
	invsqrt128(v + 128);
}
void invsqrt512(double* v) {
	invsqrt256(v);
	invsqrt256(v + 256);
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
void invsqrt_sse4(double* v) {
    invsqrt_sse2(v);
    invsqrt_sse2(v + 2);
}
void invsqrt_sse8(double* v) {
    invsqrt_sse4(v);
    invsqrt_sse4(v + 4);
}
void invsqrt_sse16(double* v) {
    invsqrt_sse8(v);
    invsqrt_sse8(v + 8);
}
void invsqrt_sse32(double* v) {
    invsqrt_sse16(v);
    invsqrt_sse16(v + 16);
}
void invsqrt_sse64(double* v) {
	invsqrt_sse32(v);
	invsqrt_sse32(v + 32);
}
void invsqrt_sse128(double* v) {
	invsqrt_sse64(v);
	invsqrt_sse64(v + 64);
}
void invsqrt_sse256(double* v) {
	invsqrt_sse128(v);
	invsqrt_sse128(v + 128);
}
void invsqrt_sse512(double* v) {
	invsqrt_sse256(v);
	invsqrt_sse256(v + 256);
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
void invsqrt_q32(double* v) {
	v[0] = invsqrt_q3(v[0]);
	v[1] = invsqrt_q3(v[1]);
}
void invsqrt_q34(double* v) {
	invsqrt_q32(v);
	invsqrt_q32(v + 2);
}
void invsqrt_q38(double* v) {
	invsqrt_q34(v);
	invsqrt_q34(v + 4);
}
void invsqrt_q316(double* v) {
	invsqrt_q38(v);
	invsqrt_q38(v + 8);
}
void invsqrt_q332(double* v) {
	invsqrt_q316(v);
	invsqrt_q316(v + 16);
}
void invsqrt_q364(double* v) {
	invsqrt_q332(v);
	invsqrt_q332(v + 32);
}
void invsqrt_q3128(double* v) {
	invsqrt_q364(v);
	invsqrt_q364(v + 64);
}
void invsqrt_q3256(double* v) {
	invsqrt_q3128(v);
	invsqrt_q3128(v + 128);
}
void invsqrt_q3512(double* v) {
	invsqrt_q3256(v);
	invsqrt_q3256(v + 256);
}

// Quake III SSE
double invsqrt_q3_sse(double v) {
    const __m128i magic_constant = _mm_set_epi64x(Q3_INVSQRT_CONSTANT, 0);
    const __m128d zero_point_five = _mm_set_pd(0.5, 0);
    const __m128d one_point_five = _mm_set_pd(1.5, 0);

    __m128d value = _mm_set_pd(v, 0);
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
void invsqrt_q3_sse4(double* v) {
    invsqrt_q3_sse2(v);
    invsqrt_q3_sse2(v + 2);
}
void invsqrt_q3_sse8(double* v) {
    invsqrt_q3_sse4(v);
    invsqrt_q3_sse4(v + 4);
}
void invsqrt_q3_sse16(double* v) {
    invsqrt_q3_sse8(v);
    invsqrt_q3_sse8(v + 8);
}
void invsqrt_q3_sse32(double* v) {
    invsqrt_q3_sse16(v);
    invsqrt_q3_sse16(v + 16);
}
void invsqrt_q3_sse64(double* v) {
	invsqrt_q3_sse32(v);
	invsqrt_q3_sse32(v + 32);
}
void invsqrt_q3_sse128(double* v) {
	invsqrt_q3_sse64(v);
	invsqrt_q3_sse64(v + 64);
}
void invsqrt_q3_sse256(double* v) {
	invsqrt_q3_sse128(v);
	invsqrt_q3_sse128(v + 128);
}
void invsqrt_q3_sse512(double* v) {
	invsqrt_q3_sse256(v);
	invsqrt_q3_sse256(v + 256);
}

typedef std::tuple<std::chrono::high_resolution_clock::duration, double, double> test_data;
typedef double(*sqrt_func_t)(double);
test_data test(double testValue, uint64_t testSize, sqrt_func_t func) {
    double x = 0;

    std::chrono::high_resolution_clock::duration t_total = std::chrono::nanoseconds(0);
    for (uint64_t run = 0; run < testSize; run++) {
        auto t_start = std::chrono::high_resolution_clock::now();
        double y = func(testValue);
        auto t_time = std::chrono::high_resolution_clock::now() - t_start;
        x += y;
        t_total += t_time;
    }

    return std::make_tuple(t_total, testValue, x);
}

typedef void(*sqrt_func_sse_t)(double*);
test_data test_sse_var(double testValue, uint64_t testSize, size_t comboSize, sqrt_func_sse_t func) {
    // Version for testing SSE on four doubles at once.

    double x = 0;
    std::vector<double> y(comboSize);
    uint64_t tmpTestSizeLoop = testSize / comboSize; // Divide by 4.
    uint64_t tmpTestSizeRem = testSize % comboSize; // Remaining parts.

    std::chrono::high_resolution_clock::duration t_total = std::chrono::nanoseconds(0);
    for (uint64_t run = 0; run < tmpTestSizeLoop; run++) {
        for (size_t vi = 0; vi < comboSize; vi++)
            y[vi] = testValue + (double)vi;

        auto t_start = std::chrono::high_resolution_clock::now();
        func(y.data());
        auto t_time = std::chrono::high_resolution_clock::now() - t_start;
        for (size_t vi = 0; vi < comboSize; vi++)
            x += y[vi];
        t_total += t_time;
    }
    for (size_t vi = 0; vi < comboSize; vi++)
        y[vi] = testValue + (double)vi;
    auto t_start = std::chrono::high_resolution_clock::now();
    func(y.data());
    auto t_time = std::chrono::high_resolution_clock::now() - t_start;
    for (size_t run = 0; run < tmpTestSizeRem; run++) {
        x += y[run];
    }
    t_total += t_time;

    return std::make_tuple(t_total, testValue, x);
}

void printLog(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::vector<char> buf(_vscprintf(format, args)+1);
    vsnprintf(buf.data(), buf.size(), format, args);
    va_end(args);
    std::cout << buf.data() << std::endl;
}

void printScore(const char* name, uint64_t timeNanoSeconds, uint64_t testSize, std::ofstream& csvfile) {
    uint64_t time_ns = timeNanoSeconds % 1000000000;
    uint64_t time_s = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::nanoseconds(timeNanoSeconds)).count() % 60;
    uint64_t time_m = std::chrono::duration_cast<std::chrono::minutes>(std::chrono::nanoseconds(timeNanoSeconds)).count() % 60;
    uint64_t time_h = std::chrono::duration_cast<std::chrono::hours>(std::chrono::nanoseconds(timeNanoSeconds)).count();
    double time_single = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::nanoseconds(timeNanoSeconds)).count() / (double)testSize;
    uint64_t pffloatfix = (uint64_t)round(time_single * 1000000);
    printLog("| %-30s | %2llu:%02llu:%02llu.%09llu | %3lld.%06lld ns | %14llu |",
        name,
        time_h, time_m, time_s, time_ns,
        pffloatfix / 1000000, pffloatfix % 1000000, // Because fuck you %3.6f, you broken piece of shit.
        (uint64_t)floor(1000000.0 / time_single)
	);
	if (csvfile.good()) {
		csvfile
			<< '"' << name << '"' << ','
			<< timeNanoSeconds << ','
			<< time_single << ','
			<< (uint64_t)floor(1000000.0 / time_single)
			<< std::endl;
	}
}

int main(int argc, const char** argv) {
    double testValue = 1234.56789;
    size_t testSize = 100000000;

    #ifdef _WIN32
    timeBeginPeriod(1);
	#endif

	std::vector<char> buf(1024);
	sprintf_s(buf.data(), 1024, "%.*s.csv", (int)strlen(argv[0]) - 4, argv[0]);
	std::ofstream csvfile(buf.data(), std::ofstream::out);
	if (csvfile.good()) {
		printLog("CSV file is written to %s.", buf.data());
	}
	if (csvfile.bad() | csvfile.fail())
		printLog("CSV file can't be written to %s.", buf.data());

    std::cout << "InvSqrt Double Test" << std::endl;
    std::cout << " - Iterations: " << testSize << std::endl;
    std::cout << " - Tested Value: " << testValue << std::endl;

	csvfile << "\"Test\",\"Time (Total)\",\"Time (Single)\",\"Score (ops/msec)\"" << std::endl;
    printLog(  "| Test Name                      | Time (Total)       | Time (Single) | Score (ops/ms) |");
    printLog(  "|:-------------------------------|-------------------:|--------------:|---------------:|");
    //printLog("| Test Name                      | Time (Total)       | Time (Single) | Score (ops/ms) |");
    //printLog("|:-------------------------------|-------------------:|--------------:|---------------:|");
    //printLog("| InvSqrt                        | 00:00:00.000000000 | 0000.00000 ns | 0000 000 000 0 |");
    //printLog("| Quake III SSE InvSqrt (2 Ops)  | ... | ... | ...");

	#define TEST(name, function) \
	{ \
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); \
		auto tv = test(testValue, testSize, function); \
		auto tvc = std::get<0>(tv); \
		printScore(name, std::chrono::duration_cast<std::chrono::nanoseconds>(tvc).count(), testSize, csvfile); \
	}
	#define TESTMULTI(name, function, size) \
	{ \
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); \
		auto tv = test_sse_var(testValue, testSize, size, function); \
		auto tvc = std::get<0>(tv); \
		printScore(name, std::chrono::duration_cast<std::chrono::nanoseconds>(tvc).count(), testSize, csvfile); \
	}

	TEST("InvSqrt", invsqrt);
	TESTMULTI("InvSqrt (2 Ops)", invsqrt2, 2);
	TESTMULTI("InvSqrt (4 Ops)", invsqrt4, 4);
	TESTMULTI("InvSqrt (8 Ops)", invsqrt8, 8);
	TESTMULTI("InvSqrt (16 Ops)", invsqrt16, 16);
	TESTMULTI("InvSqrt (32 Ops)", invsqrt32, 32);
	TESTMULTI("InvSqrt (64 Ops)", invsqrt64, 64);
	TESTMULTI("InvSqrt (128 Ops)", invsqrt128, 128);
	TESTMULTI("InvSqrt (256 Ops)", invsqrt256, 256);
	TESTMULTI("InvSqrt (512 Ops)", invsqrt512, 512);
	printLog("|:-------------------------------|-------------------:|--------------:|---------------:|");
	TEST("InvSqrt SSE", invsqrt_sse);
	TESTMULTI("InvSqrt SSE (2 Ops)", invsqrt_sse2, 2);
	TESTMULTI("InvSqrt SSE (4 Ops)", invsqrt_sse4, 4);
	TESTMULTI("InvSqrt SSE (8 Ops)", invsqrt_sse8, 8);
	TESTMULTI("InvSqrt SSE (16 Ops)", invsqrt_sse16, 16);
	TESTMULTI("InvSqrt SSE (32 Ops)", invsqrt_sse32, 32);
	TESTMULTI("InvSqrt SSE (64 Ops)", invsqrt_sse64, 64);
	TESTMULTI("InvSqrt SSE (128 Ops)", invsqrt_sse128, 128);
	TESTMULTI("InvSqrt SSE (266 Ops)", invsqrt_sse256, 256);
	TESTMULTI("InvSqrt SSE (512 Ops)", invsqrt_sse512, 512);
	printLog("|:-------------------------------|-------------------:|--------------:|---------------:|");
	TEST("Q3 InvSqrt", invsqrt_q3);
	TESTMULTI("Q3 InvSqrt (2 Ops)", invsqrt_q32, 2);
	TESTMULTI("Q3 InvSqrt (4 Ops)", invsqrt_q34, 4);
	TESTMULTI("Q3 InvSqrt (8 Ops)", invsqrt_q38, 8);
	TESTMULTI("Q3 InvSqrt (16 Ops)", invsqrt_q316, 16);
	TESTMULTI("Q3 InvSqrt (32 Ops)", invsqrt_q332, 32);
	TESTMULTI("Q3 InvSqrt (64 Ops)", invsqrt_q364, 64);
	TESTMULTI("Q3 InvSqrt (128 Ops)", invsqrt_q3128, 128);
	TESTMULTI("Q3 InvSqrt (256 Ops)", invsqrt_q3256, 256);
	TESTMULTI("Q3 InvSqrt (512 Ops)", invsqrt_q3512, 512);
	printLog("|:-------------------------------|-------------------:|--------------:|---------------:|");
	TEST("Q3 InvSqrt SSE", invsqrt_q3_sse);
	TESTMULTI("Q3 InvSqrt SSE (2 Ops)", invsqrt_q3_sse2, 2);
	TESTMULTI("Q3 InvSqrt SSE (4 Ops)", invsqrt_q3_sse4, 4);
	TESTMULTI("Q3 InvSqrt SSE (8 Ops)", invsqrt_q3_sse8, 8);
	TESTMULTI("Q3 InvSqrt SSE (16 Ops)", invsqrt_q3_sse16, 16);
	TESTMULTI("Q3 InvSqrt SSE (32 Ops)", invsqrt_q3_sse32, 32);
	TESTMULTI("Q3 InvSqrt SSE (64 Ops)", invsqrt_q3_sse64, 64);
	TESTMULTI("Q3 InvSqrt SSE (128 Ops)", invsqrt_q3_sse128, 128);
	TESTMULTI("Q3 InvSqrt SSE (256 Ops)", invsqrt_q3_sse256, 256);
	TESTMULTI("Q3 InvSqrt SSE (512 Ops)", invsqrt_q3_sse512, 512);

	csvfile.close();

    #ifdef _WIN32
    timeEndPeriod(1);
    #endif

    getchar();
    return 0;
}

