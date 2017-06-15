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
#include <sparsepp/spp_bitset.h>

using namespace std;

// -----------------------------------------------------------
// -----------------------------------------------------------
template <size_t N>
class TestBitset
{
public:
    typedef spp::spp_bitset<N> BS;

    TestBitset()
    {}

    void test_set(size_t num_iter)
    {
        size_t num_errors = 0;
        BS bs, bs2;

        printf("testing set on spp_bitset<%zu>  , num_iter=%6zu -> ", N, num_iter);

        for (size_t i=0; i<num_iter; ++i)
        {
            bs.reset();
            bs2.reset();
            size_t start = rand() % N;
            size_t to = start + rand() % (N - start);  
            bs.set(start, to);
            bs2.set_naive(start, to);
            bool same = bs == bs2;
            if (!same)
                ++num_errors;
            assert(same);
        }
        printf("num_errors = %zu\n", num_errors);
    }

    void test_reset(size_t num_iter)
    {
        size_t num_errors = 0;
        BS bs, bs2;
        printf("testing reset on spp_bitset<%zu>, num_iter=%6zu -> ", N, num_iter);

        for (size_t i=0; i<num_iter; ++i)
        {
            bs.set();
            bs2.set();
            size_t start = rand() % N;
            size_t to = start + rand() % (N - start);  
            bs.reset(start, to);
            bs2.reset_naive(start, to);
            bool same = bs == bs2;
            if (!same)
                ++num_errors;
            assert(same);
        }
        printf("num_errors = %zu\n", num_errors);
    }

    void test_all(size_t num_iter)
    {
        size_t num_errors = 0;
        BS bs;
        printf("testing all() on spp_bitset<%zu>, num_iter=%6zu -> ", N, num_iter);

        for (size_t i=0; i<4 * N; ++i)
        {
            bs.set(rand() % N);
            if (i > 2 * N)
            {
                for (size_t j=0; j<num_iter; ++j)
                {
                    size_t start = rand() % N;
                    size_t to = start + rand() % (N - start);  
                    bool same = bs.all(start, to) == bs.all_naive(start, to);
                    if (!same)
                        ++num_errors;
                    assert(same);                  
                }

                size_t start = 0, start_naive = 1;
                bs.all(start);
                bs.all_naive(start_naive);
                bool same = (start == start_naive);
                if (!same)
                    ++num_errors;
                assert(same);   
            }
        }
        printf("num_errors = %zu\n", num_errors);
    }

    void test_any(size_t num_iter)
    {
        size_t num_errors = 0;
        BS bs;
        printf("testing any() on spp_bitset<%zu>, num_iter=%6zu -> ", N, num_iter);

        for (size_t i=0; i<num_iter; ++i)
        {
            bs.set(rand() % N);
            for (size_t j=0; j<100; ++j)
            {
                size_t start = rand() % N;
                size_t to = start + rand() % (N - start);  
                bool same = bs.any(start, to) == bs.any_naive(start, to);
                if (!same)
                    ++num_errors;
                assert(same);      
            }
        }
        printf("num_errors = %zu\n", num_errors);
    }

    void test_longest(size_t num_iter)
    {
        size_t num_errors = 0;
        BS bs, bs2;
        assert(bs.longest_zero_sequence() == N);
        bs.set(0);
        assert(bs.longest_zero_sequence() == N-1);
        bs.set(10);
        assert(bs.find_next_n(3, 8) == 11);
        assert(bs.find_next_n(3, 6) == 6);
        assert(bs.find_next_n(3, N-2) == 1);
        assert(bs.longest_zero_sequence() == N-11);
        if (N > 1000)
        {
            bs.set(1000);
            size_t longest = bs.longest_zero_sequence();
            assert(longest == 1000-11 || longest == N-1001);
            if (!(longest == 1000-11 || longest == N-1001))
                ++num_errors;
        }

        spp::Timer<std::milli> timer_lz;
        spp::Timer<std::milli> timer_lz_slow;
        float lz_time(0), lz_time_slow(0);

        printf("testing longest_zero_sequence()  , num_iter=%6zu -> ", num_iter);
        srand(1);
        for (size_t i=0; i<num_iter; ++i)
        {
            bs.reset();
            for (size_t j=0; j<N; ++j)
            {
                bs.set(rand() % N);

                timer_lz.snap();
                size_t lz1 = bs.longest_zero_sequence();
                lz_time += timer_lz.get_delta();

                timer_lz_slow.snap();
                size_t lz2 = bs.longest_zero_sequence_naive();
                lz_time_slow += timer_lz_slow.get_delta();

                num_errors += (lz1 != lz2);
                assert(!num_errors);
            }
        } 

       printf("num_errors = %zu, time=%7.1f, slow_time=%7.1f\n", num_errors, lz_time, lz_time_slow); 
    }

    void test_longest2(size_t num_iter)
    {
        size_t num_errors = 0;
        BS bs, bs2;
        assert(bs.longest_zero_sequence() == N);
        bs.set(0);
        assert(bs.longest_zero_sequence() == N-1);
        bs.set(10);
        assert(bs.find_next_n(3, 8) == 11);
        assert(bs.find_next_n(3, 6) == 6);
        assert(bs.find_next_n(3, N-2) == 1);
        assert(bs.longest_zero_sequence() == N-11);
        if (N > 1000)
        {
            bs.set(1000);
            size_t longest = bs.longest_zero_sequence();
            assert(longest == 1000-11 || longest == N-1001);
            if (!(longest == 1000-11 || longest == N-1001))
                ++num_errors;
        }

        spp::Timer<std::milli> timer_lz;
        spp::Timer<std::milli> timer_lz_slow;
        float lz_time(0), lz_time_slow(0);

        printf("testing longest_zero_sequence2() , num_iter=%6zu -> ", num_iter);
        srand(1);
        for (size_t i=0; i<num_iter; ++i)
        {
            bs.reset();
            for (size_t j=0; j<N; ++j)
            {
                bs.set(rand() % N);
                size_t start_pos1 = 0, start_pos2 = 0;

                timer_lz.snap();
                size_t lz1 = bs.longest_zero_sequence(64, start_pos1);
                lz_time += timer_lz.get_delta();

                timer_lz_slow.snap();
                size_t lz2 = bs.longest_zero_sequence_naive(64, start_pos2);
                lz_time_slow += timer_lz_slow.get_delta();
                
                assert(start_pos1 == start_pos2);

                num_errors += (lz1 != lz2) || (start_pos1 != start_pos2);
                assert(!num_errors);
            }
        } 

       printf("num_errors = %zu, time=%7.1f, slow_time=%7.1f\n", num_errors, lz_time, lz_time_slow); 
    }

    void test_ctz(size_t num_iter) 
    {
        size_t num_errors = 0;

        spp::Timer<std::milli> timer_ctz;
        spp::Timer<std::milli> timer_ctz_slow;
        float ctz_time(0), ctz_time_slow(0);

        printf("testing count_trailing_zeroes()  , num_iter=%6zu -> ", num_iter);
        for (size_t i=0; i<num_iter; ++i)
        {
            size_t v = rand() ^ (rand() << 16);

            timer_ctz.snap();
            uint32_t ctz1 = spp::count_trailing_zeroes(v);
            ctz_time += timer_ctz.get_delta();

            timer_ctz_slow.snap();
            size_t ctz2 = spp::count_trailing_zeroes_naive(v);
            ctz_time_slow += timer_ctz_slow.get_delta();

            num_errors += (ctz1 != ctz2);
            assert(!num_errors);
        } 

        printf("num_errors = %zu, time=%7.1f, slow_time=%7.1f\n", num_errors, ctz_time, ctz_time_slow); 
            
    }

    void run()
    {
        test_ctz(10000);
        test_all(10000);
        test_any(1000);
        test_set(1000);
        test_reset(1000);
        test_longest(200);
        test_longest2(200);
    }
};

// -----------------------------------------------------------
// -----------------------------------------------------------
int main()
{
    TestBitset<1024> test_bitset_1024;
    test_bitset_1024.run();

    TestBitset<4096> test_bitset_4096;
    test_bitset_4096.run();

    //TestBitset<8192> test_bitset_8192;
    //test_bitset_8192.run();
}
