// advmemcpy.cpp : Defines the entry point for the console application.
//

#include <asmlib.h>
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
#include <intrin.h>
#include "apex_memmove.h"
#include "measurer.hpp"
#include "memcpy_adv.h"

#undef max

#define MEASURE_TEST_CYCLES 1000

#define SIZE(W, H, C, N) \
	{ \
		W *H *C, N \
	}

using namespace std;
/**
 * Allocator for aligned data.
 *
 * Modified from the Mallocator from Stephan T. Lavavej.
 * <http://blogs.msdn.com/b/vcblog/archive/2008/08/28/the-mallocator.aspx>
 */
template<typename T, std::size_t Alignment>
class aligned_allocator {
	public:
	// The following will be the same for virtually all allocators.
	typedef T*          pointer;
	typedef const T*    const_pointer;
	typedef T&          reference;
	typedef const T&    const_reference;
	typedef T           value_type;
	typedef std::size_t size_type;
	typedef ptrdiff_t   difference_type;

	T* address(T& r) const
	{
		return &r;
	}

	const T* address(const T& s) const
	{
		return &s;
	}

	std::size_t max_size() const
	{
		// The following has been carefully written to be independent of
		// the definition of size_t and to avoid signed/unsigned warnings.
		return (static_cast<std::size_t>(0) - static_cast<std::size_t>(1)) / sizeof(T);
	}

	// The following must be the same for all allocators.
	template<typename U>
	struct rebind {
		typedef aligned_allocator<U, Alignment> other;
	};

	bool operator!=(const aligned_allocator& other) const
	{
		return !(*this == other);
	}

	void construct(T* const p, const T& t) const
	{
		void* const pv = static_cast<void*>(p);

		new (pv) T(t);
	}

	void destroy(T* const p) const
	{
		p->~T();
	}

	// Returns true if and only if storage allocated from *this
	// can be deallocated from other, and vice versa.
	// Always returns true for stateless allocators.
	bool operator==(const aligned_allocator& other) const
	{
		return true;
	}

	// Default constructor, copy constructor, rebinding constructor, and destructor.
	// Empty for stateless allocators.
	aligned_allocator() {}

	aligned_allocator(const aligned_allocator&) {}

	template<typename U>
	aligned_allocator(const aligned_allocator<U, Alignment>&)
	{}

	~aligned_allocator() {}

	// The following will be different for each allocator.
	T* allocate(const std::size_t n) const
	{
		// The return value of allocate(0) is unspecified.
		// Mallocator returns NULL in order to avoid depending
		// on malloc(0)'s implementation-defined behavior
		// (the implementation can define malloc(0) to return NULL,
		// in which case the bad_alloc check below would fire).
		// All allocators can return NULL in this case.
		if (n == 0) {
			return NULL;
		}

		// All allocators should contain an integer overflow check.
		// The Standardization Committee recommends that std::length_error
		// be thrown in the case of integer overflow.
		if (n > max_size()) {
			throw std::length_error("aligned_allocator<T>::allocate() - Integer overflow.");
		}

		// Mallocator wraps malloc().
		void* const pv = _mm_malloc(n * sizeof(T), Alignment);

		// Allocators should throw std::bad_alloc in the case of memory allocation failure.
		if (pv == NULL) {
			throw std::bad_alloc();
		}

		return static_cast<T*>(pv);
	}

	void deallocate(T* const p, const std::size_t n) const
	{
		_mm_free(p);
	}

	// The following will be the same for all allocators that ignore hints.
	template<typename U>
	T* allocate(const std::size_t n, const U* /* const hint */) const
	{
		return allocate(n);
	}

	// Allocators are not required to be assignable, so
	// all allocators should have a private unimplemented
	// assignment operator. Note that this will trigger the
	// off-by-default (enabled under /Wall) warning C4626
	// "assignment operator could not be generated because a
	// base class assignment operator is inaccessible" within
	// the STL headers, but that warning is useless.
	private:
	aligned_allocator& operator=(const aligned_allocator&) = delete;
};

std::map<size_t, std::string> test_sizes{
    //SIZE(1, 1, 1024, "1KB"),
    //SIZE(1, 2, 1024, "2KB"),
    //SIZE(1, 4, 1024, "4KB"),
    //SIZE(1, 8, 1024, "8KB"),
    //SIZE(1, 16, 1024, "16KB"),
    //SIZE(1, 32, 1024, "32KB"),
    //SIZE(1, 64, 1024, "64KB"),
    //SIZE(1, 128, 1024, "128KB"),
    //SIZE(1, 256, 1024, "256KB"),
    //SIZE(1, 512, 1024, "512KB"),
    //SIZE(1, 1024, 1024, "1MB"),   SIZE(2, 1024, 1024, "2MB"),   SIZE(3, 1024, 1024, "2MB"),
    //SIZE(4, 1024, 1024, "4MB"),   SIZE(5, 1024, 1024, "8MB"),   SIZE(6, 1024, 1024, "8MB"),
    //SIZE(7, 1024, 1024, "8MB"),   SIZE(8, 1024, 1024, "8MB"),   SIZE(16, 1024, 1024, "16MB"),
    //SIZE(32, 1024, 1024, "32MB"), SIZE(64, 1024, 1024, "64MB"),
    SIZE(1280, 720, 2, "1280x720 NV12"),
    SIZE(1980, 1080, 2, "1920x1080 NV12"),
    SIZE(2560, 1440, 2, "2560x1440 NV12"),
    SIZE(3840, 2160, 2, "3840x2160 NV12"),
};

std::map<std::string, std::function<void*(void* to, void* from, size_t size)>> functions{
    {"A_memcpy", A_memcpy},
    {"memcpy", &std::memcpy},
    //{ "movsb", &memcpy_movsb },
    //{ "movsw", &memcpy_movsw },
    //{ "movsd", &memcpy_movsd },
    //{ "movsq", &memcpy_movsq },
    {"apex_memcpy", [](void* t, void* f, size_t s) { return apex::memcpy(t, f, s); }},
    //{ "advmemcpy", &memcpy_thread },
    //{ "advmemcpy_apex", &memcpy_thread },
};

std::map<std::string, std::function<void()>> initializers{
    {"apex_memcpy", []() { apex::memcpy(0, 0, 0); }},
    {"A_memcpy", []() {}},
};

int32_t main(int32_t argc, const char* argv[])
{
	int64_t rw1, rw2, rw3, rw4, rw5, rw6, rw7, rw8;

	std::vector<uint8_t, aligned_allocator<uint8_t, 32>> buf_from, buf_to, cache_kill;

	size_t largest_size = 0;
	for (auto test : test_sizes) {
		if (test.first > largest_size)
			largest_size = test.first;
	}
	buf_from.resize(largest_size * 4);
	buf_to.resize(largest_size * 4);
	cache_kill.resize(64 * 1024 * 1024);

	srand(static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()
	                            % 0xFFFFFFFFull));

	measurer fence, fenc2, flush;

	std::cin.get();
	for (auto test : test_sizes) {
		std::cout << "Testing '" << test.second << "' ( " << (test.first) << " B )..." << std::endl;
		std::cout << "Name            | Avg.  MB/s | 95.0% MB/s | 99.0% MB/s | 99.9% MB/s " << std::endl
		          << "----------------+------------+------------+------------+------------" << std::endl;

		size_t size = test.first;
		for (auto func : functions) {
			measurer measure;

			auto inits = initializers.find(func.first);
			if (inits != initializers.end()) {
				inits->second();
			}

			std::cout << setw(16) << setiosflags(ios::left) << func.first;
			std::cout << setw(0) << resetiosflags(ios::left) << "|" << std::flush;

			for (size_t idx = 0; idx < MEASURE_TEST_CYCLES; idx++) {
				// Get a random address to work from, but don't drop the 32-byte alignment.
				uint8_t* from = buf_from.data() + ((rand() % largest_size * 3) & ~0b011111);
				uint8_t* to   = buf_to.data() + ((rand() % largest_size * 3) & ~0b011111);

				// Make sure the entire address space is invalidated and flushed from cache.
				{
					auto tracker = flush.track();
					for (size_t sz = 0; sz < size; sz++) {
						_mm_clflush(from + sz);
						cache_kill.at(rand() % cache_kill.size()) =
						    rand() % std::numeric_limits<uint8_t>::max();
					}
				}

				// Fence to avoid any incorrect late load/stores that can affect timings.
				{
					auto tracker = fence.track();
					_mm_mfence();
				}

				{
					auto tracker = measure.track();
					func.second(to, from, size);
				}

				// Fence to avoid any incorrect late load/stores that can affect timings.
				{
					auto tracker = fenc2.track();
					_mm_mfence();
				}

				// Fence to avoid any incorrect late load/stores that can affect timings.
				// ToDo: Should this actually be in the loop?
				//_mm_mfence();

				// Clear Insutruction Caches and likely have an impact on branch caches.
				for (size_t idx2 = 0; idx < 100; idx++) {
					rw1 += rw2;
					rw2 -= rw3 * rw7;
					rw8 = rw2 - rw1;
					rw6 = rw3 + rw8;
					rw5 = rw4 / rw3;
					rw2 *= rw8;
					rw3++;
					if (rw1 == rw2) {
						rw2 = rw3;
					} else {
						rw1 = rw2;
					}
					if (rw2)
						rw2 = rw6;
					else
						rw6 = rw2;
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

	std::cout << "Name            | Avg. �s    | 95.0% �s   | 99.0% �s   | 99.9% �s   \n"
	          << "----------------+------------+------------+------------+------------\n";

	std::cout << setw(16) << setiosflags(ios::left) << "_mm_clflush" << setw(0) << resetiosflags(ios::left) << "|"
	          << setw(11) << setprecision(3) << setiosflags(ios::right) << std::fixed
	          << flush.average_duration() / 1000 << setw(0) << resetiosflags(ios::right) << " |" << setw(11)
	          << setprecision(3) << setiosflags(ios::right) << std::fixed << flush.percentile(0.95).count() / 1000.0
	          << setw(0) << resetiosflags(ios::right) << " |" << setw(11) << setprecision(3)
	          << setiosflags(ios::right) << std::fixed << flush.percentile(0.99).count() / 1000.0 << setw(0)
	          << resetiosflags(ios::right) << " |" << setw(11) << setprecision(3) << setiosflags(ios::right)
	          << std::fixed << flush.percentile(0.999).count() / 1000.0 << setw(0) << resetiosflags(ios::right)
	          << '\n';
	std::cout << setw(16) << setiosflags(ios::left) << "_mm_mfence1" << setw(0) << resetiosflags(ios::left) << "|"
	          << setw(11) << setprecision(3) << setiosflags(ios::right) << std::fixed
	          << fence.average_duration() / 1000 << setw(0) << resetiosflags(ios::right) << " |" << setw(11)
	          << setprecision(3) << setiosflags(ios::right) << std::fixed << fence.percentile(0.95).count() / 1000.0
	          << setw(0) << resetiosflags(ios::right) << " |" << setw(11) << setprecision(3)
	          << setiosflags(ios::right) << std::fixed << fence.percentile(0.99).count() / 1000.0 << setw(0)
	          << resetiosflags(ios::right) << " |" << setw(11) << setprecision(3) << setiosflags(ios::right)
	          << std::fixed << fence.percentile(0.999).count() / 1000.0 << setw(0) << resetiosflags(ios::right)
	          << '\n';
	std::cout << setw(16) << setiosflags(ios::left) << "_mm_mfence2" << setw(0) << resetiosflags(ios::left) << "|"
	          << setw(11) << setprecision(3) << setiosflags(ios::right) << std::fixed
	          << fenc2.average_duration() / 1000 << setw(0) << resetiosflags(ios::right) << " |" << setw(11)
	          << setprecision(3) << setiosflags(ios::right) << std::fixed << fenc2.percentile(0.95).count() / 1000.0
	          << setw(0) << resetiosflags(ios::right) << " |" << setw(11) << setprecision(3)
	          << setiosflags(ios::right) << std::fixed << fenc2.percentile(0.99).count() / 1000.0 << setw(0)
	          << resetiosflags(ios::right) << " |" << setw(11) << setprecision(3) << setiosflags(ios::right)
	          << std::fixed << fenc2.percentile(0.999).count() / 1000.0 << setw(0) << resetiosflags(ios::right)
	          << '\n';

	std::cout << std::flush << std::endl;
	std::cin.get();
	return 0;
}
