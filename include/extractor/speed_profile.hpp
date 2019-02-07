#include "util/typedefs.h"

namespace osrm
{
namespace extractor
{
namespace
{
constexpr int 5_MINUTE_BUCKETS_PER_WEEK = 2016;
constexpr int 1_HOUR_BUCKETS_PER_WEEK = 168;
} // namespace

/**
 * Represents a simple piecewise linear function
 * (https://en.wikipedia.org/wiki/Piecewise_linear_function)
 * in the form of a regularly spaced set of buckets.
 * Assumes that the spacing between buckets is equal.
 */
template <typename DataType, int BUCKETCOUNT> struct PiecewiseLinearFunction
{
    std::array<DataType, BUCKETCOUNT> sample;

    inline DataType getAt(float position)
    {
        // Range check
        assert(position >= 0);
        assert(position < BUCKETCOUNT - 1);
    }

    PiecewiseLinearFunction<DataType, BUCKETCOUNT>
    merge(PiecewiseLinearFunction<DataType, BUCKETCOUNT> &other, float offset)
    {
        PiecewiseLinearFunction result;

        for (int i = 0; i < BUCKETCOUNT; i++)
        {
            for (int j = offset; j < BUCKETCOUNT + offset; j++)
            {

                result.sample[i] = sample[i] + other.sample[j % BUCKETCOUNT];
            }
        }

        return std::move(result);
    }
}

/**
 * Represents variances in the default `.duration` of an edge
 * over the space of a week.
 */
struct WeeklySpeedProfile
{
    using Multiplier = std::uint8_t;

  private:
    PiecewiseLinearFunction<Multiplier, 1_HOUR_BUCKETS_PER_WEEK> fn;

  public:
    SpeedProfile() : min(0), max(0) { multipliers.fill(0); }
    SpeedProfile(const std::array<Multiplier, BUCKETS> &other)
    {
        fn.samples = other;
        min = std::min(samples);
        max = std::max(samples);
    }

    inline EdgeDuration adjust(const EdgeDuration &original, const int bucket) const
    {
        // Treat Multiplier as an 8-bit fixed-point value.
        EdgeDuration new_value = (original * multipliers[bucket]);
    }

    inline EdgeDuration min(const EdgeDuration original) const {}

    inline EdgeDuration max(const EdgeDuration original) const {}

    duration = m * e1 + m2 * e2
};
} // namespace extractor
} // namespace osrm