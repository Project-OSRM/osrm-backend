#ifndef OSRM_UTIL_TIMED_HISTOGRAM_HPP
#define OSRM_UTIL_TIMED_HISTOGRAM_HPP

#include "util/integer_range.hpp"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <sstream>
#include <vector>

namespace osrm
{
namespace util
{
namespace detail
{
extern std::atomic_uint operation;
}

/**
 * Captures a histogram with a bin size of `IndexBinSize` every `TimeBinSize` count operations.
 */
template <std::size_t TimeBinSize = 1000, std::size_t IndexBinSize = 1000> class TimedHistogram
{
  public:
    void Count(std::size_t pos)
    {
        std::lock_guard<std::mutex> guard(frames_lock);
        auto frame_index = detail::operation++ / TimeBinSize;

        while (frame_offsets.size() <= frame_index)
        {
            frame_offsets.push_back(frame_counters.size());
        }
        BOOST_ASSERT(frame_offsets.size() == frame_index + 1);

        auto frame_offset = frame_offsets.back();
        auto counter_index = frame_offset + pos / IndexBinSize;

        while (counter_index >= frame_counters.size())
        {
            frame_counters.push_back(0);
        }

        BOOST_ASSERT(frame_counters.size() > counter_index);
        frame_counters[counter_index]++;
    }

    // Returns the measurments as a CSV file with the columns:
    // frame_id,index_bin,count
    std::string DumpCSV() const
    {
        std::stringstream out;

        const auto print_bins = [&out](auto frame_index, auto begin, auto end) {
            auto bin_index = 0;
            std::for_each(begin, end, [&](const auto count) {
                if (count > 0)
                {
                    out << (frame_index * TimeBinSize) << "," << (bin_index * IndexBinSize) << ","
                        << count << std::endl;
                }
                bin_index++;
            });
        };

        if (frame_offsets.size() == 0)
        {
            return "";
        }

        for (const auto frame_index : irange<std::size_t>(0, frame_offsets.size() - 1))
        {
            auto begin = frame_counters.begin() + frame_offsets[frame_index];
            auto end = frame_counters.begin() + frame_offsets[frame_index + 1];
            print_bins(frame_index, begin, end);
        }
        print_bins(frame_offsets.size() - 1,
                   frame_counters.begin() + frame_offsets.back(),
                   frame_counters.end());

        return out.str();
    }

  private:
    std::mutex frames_lock;
    std::vector<std::uint32_t> frame_offsets;
    std::vector<std::uint32_t> frame_counters;
};
}
}

#endif
