#include <memory>
#include <cassert>
#include <cstdio>
#include <stdlib.h> 
#include <algorithm> 
#include <vector>

// enable debugging code in spp_bitset.h
#define SPP_TEST 1

#include <sparsepp/spp_timer.h>
#include <sparsepp/spp_memory.h>
#include <sparsepp/spp_dlalloc.h>

using namespace std;

static float _to_mb(uint64_t m) { return (float)((double)m / (1024 * 1024)); }

// -----------------------------------------------------------
// -----------------------------------------------------------
template <class T, class A>
class TestAlloc
{
public:
    TestAlloc(size_t num_alloc = 8000000) : 
        _num_alloc(num_alloc)
    {
        _allocated.resize(_num_alloc, nullptr);
        _sizes.resize(_num_alloc, 0);
        _start_mem_usage = spp::GetProcessMemoryUsed();
    }

    void run()
    {
        srand(43); // always same sequence of random numbers

        for (size_t i=0; i<_num_alloc; ++i)
            _sizes[i] = std::max(2, (rand() % 5) * 2);
                                 
        spp::Timer<std::milli> timer;

        // allocate small buffers
        // ----------------------
        for (size_t i=0; i<_num_alloc; ++i)
        {
            _allocated[i] = _allocator.allocate(_sizes[i]);
            _set_buf(_allocated[i], _sizes[i]);
        }
        
#if 1
        // and grow the buffers to a max size of 24 each
        // ---------------------------------------------
        for (uint32_t j=4; j<26; j += 2)
        {
            for (size_t i=0; i<_num_alloc; ++i)
            {
                // if ( _sizes[i] < j)                    // windows allocator friendly!
                if ((rand() % 4) != 3 && _sizes[i] < j)   // really messes up windows allocator
                {
                    _allocated[i] = _allocator.reallocate(_allocated[i], j);
                    _check_buf(_allocated[i], _sizes[i]);
                    _set_buf(_allocated[i], j);
                    _sizes[i] = j;
                }
            }
        }
#endif

#if 0
        // test erase (shrinking the buffers)
        // ---------------------------------------------
        for (uint32_t j=28; j>4; j -= 2)
        {
            for (size_t i=0; i<_num_alloc; ++i)
            {
                // if ( _sizes[i] < j)                    // windows allocator friendly!
                if ((rand() % 4) != 3 && _sizes[i] > j)   // really messes up windows allocator
                {
                    _allocated[i] = _allocator.reallocate(_allocated[i], j);
                    _check_buf1(_allocated[i], _sizes[i]);
                    _set_buf(_allocated[i], j);
                    _sizes[i] = j;
                }
            }
        }
#endif

#if 0
        // and grow the buffers back to a max size of 24 each
        // --------------------------------------------------
        for (uint32_t j=4; j<26; j += 2)
        {
            for (size_t i=0; i<_num_alloc; ++i)
            {
                // if ( _sizes[i] < j)                    // windows allocator friendly!
                if ((rand() % 4) != 3 && _sizes[i] < j)   // really messes up windows allocator
                {
                    _allocated[i] = _allocator.reallocate(_allocated[i], j);
                    _check_buf(_allocated[i], _sizes[i]);
                    _set_buf(_allocated[i], j);
                    _sizes[i] = j;
                }
            }
        }
#endif

        size_t total_units = 0;
        for (size_t i=0; i<_num_alloc; ++i)
            total_units += _sizes[i];
        
        uint64_t mem_usage          = spp::GetProcessMemoryUsed();
        uint64_t alloc_mem_usage    = mem_usage - _start_mem_usage;
        uint64_t expected_mem_usage = total_units * sizeof(T);

        // finally free the memory
        // -----------------------
        for (size_t i=0; i<_num_alloc; ++i)
        {
            _check_buf(_allocated[i], _sizes[i]);
            _allocator.deallocate(_allocated[i], _sizes[i]);
        }

        uint64_t mem_usage_end = spp::GetProcessMemoryUsed();

        printf("allocated %zd entities of size %zd\n", total_units, sizeof(T));
        printf("done in %3.2f seconds, mem_usage %4.1f/%4.1f/%4.1f MB\n", 
               timer.get_total() / 1000, _to_mb(_start_mem_usage),  _to_mb(mem_usage),  _to_mb(mem_usage_end));
        printf("expected mem usage: %4.1f\n", _to_mb(expected_mem_usage));
        if (expected_mem_usage <= alloc_mem_usage)
            printf("overhead: %4.1f%%\n", 
                   (float)((double)(alloc_mem_usage - expected_mem_usage) / expected_mem_usage) * 100);
        else
            printf("bug: alloc_mem_usage <= expected_mem_usage\n");
        
        std::vector<T *>().swap(_allocated);
        std::vector<uint32_t>().swap(_sizes);

        printf("\nmem usage after freeing vectors: %4.1f\n", _to_mb(spp::GetProcessMemoryUsed()));
    }

private:

    void _set_buf(T *buff, uint32_t sz) { *buff = (T)sz; buff[sz - 1] = (T)sz; }
    void _check_buf1(T *buff, uint32_t sz) 
    { 
        assert(*buff == (T)sz); 
        (void)(buff + sz); // silence warning
    }
    void _check_buf(T *buff, uint32_t sz) 
    { 
        assert(*buff == (T)sz &&  buff[sz - 1] == (T)sz); 
        (void)(buff + sz); // silence warning
    }

    size_t                _num_alloc;
    uint64_t              _start_mem_usage;
    std::vector<T *>      _allocated;
    std::vector<uint32_t> _sizes;
    A                     _allocator;
};

// -----------------------------------------------------------
// -----------------------------------------------------------
template <class X, class A>
void run_test(const char *alloc_name)
{
    printf("\n---------------- testing %s\n\n", alloc_name);

    printf("\nmem usage before the alloc test: %4.1f\n", 
           _to_mb(spp::GetProcessMemoryUsed()));
    {
        TestAlloc< X, A >  test_alloc;
        test_alloc.run();
    }
    printf("mem usage after the alloc test: %4.1f\n",
           _to_mb(spp::GetProcessMemoryUsed()));

    printf("\n\n");
}

// -----------------------------------------------------------
// -----------------------------------------------------------
int main()
{
    typedef uint64_t X;

    run_test<X, spp::libc_allocator<X>>("libc_allocator");
    run_test<X, spp::spp_allocator<X>>("spp_allocator");
}
