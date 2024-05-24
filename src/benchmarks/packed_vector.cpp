#include "util/packed_vector.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/timing_util.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace osrm;

struct Measurement
{
    double random_write_ms;
    double random_read_ms;
};

#ifdef _WIN32
#pragma optimize("", off)
template <class T> void dont_optimize_away(T &&datum) { T local = datum; }
#pragma optimize("", on)
#else
template <class T> void dont_optimize_away(T &&datum) { asm volatile("" : "+r"(datum)); }
#endif

template <std::size_t num_rounds, std::size_t num_entries, typename VectorT>
auto measure_random_access()
{
    std::vector<std::size_t> indices(num_entries);
    std::iota(indices.begin(), indices.end(), 0);
    std::mt19937 g(1337);
    std::shuffle(indices.begin(), indices.end(), g);

    VectorT vector(num_entries);

    TIMER_START(write);
    for (auto round : util::irange<std::size_t>(0, num_rounds))
    {
        for (auto idx : util::irange<std::size_t>(0, num_entries))
        {
            vector[indices[idx]] = idx + round;
        }
    }
    TIMER_STOP(write);

    TIMER_START(read);
    auto sum = 0;
    for (auto round : util::irange<std::size_t>(0, num_rounds))
    {
        sum = round;
        for (auto idx : util::irange<std::size_t>(0, num_entries))
        {
            sum += vector[indices[idx]];
        }
        dont_optimize_away(sum);
    }
    TIMER_STOP(read);

    return Measurement{TIMER_MSEC(write), TIMER_MSEC(read)};
}

int main(int, char **)
{
    util::LogPolicy::GetInstance().Unmute();

    auto result_plain = measure_random_access<10000, 1000000, std::vector<std::uint32_t>>();
    auto result_packed =
        measure_random_access<10000, 1000000, util::PackedVector<std::uint32_t, 22>>();

    auto write_slowdown = result_packed.random_write_ms / result_plain.random_write_ms;
    auto read_slowdown = result_packed.random_read_ms / result_plain.random_read_ms;
    std::cout << "random write:\nstd::vector " << result_plain.random_write_ms
              << " ms\nutil::packed_vector " << result_packed.random_write_ms << " ms\n"
              << "slowdown: " << write_slowdown << std::endl;
    std::cout << "random read:\nstd::vector " << result_plain.random_read_ms
              << " ms\nutil::packed_vector " << result_packed.random_read_ms << " ms\n"
              << "slowdown: " << read_slowdown << std::endl;
}
