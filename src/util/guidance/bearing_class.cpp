#include "extractor/guidance/discrete_angle.hpp"
#include "util/guidance/bearing_class.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace util
{
namespace guidance
{

static_assert(
    360 / BearingClass::discrete_angle_step_size <= 8 * sizeof(BearingClass::FlagBaseType),
    "The number of expressable bearings does not fit into the datatype used for storage.");

std::uint8_t BearingClass::angleToDiscreteID(double angle)
{
    BOOST_ASSERT(angle >= 0. && angle <= 360.);
    // shift angle by half the step size to have the class be located around the center
    angle = (angle + 0.5 * BearingClass::discrete_angle_step_size);
    if (angle > 360)
        angle -= 360;

    return std::uint8_t(angle / BearingClass::discrete_angle_step_size);
}

double BearingClass::discreteIDToAngle(std::uint8_t id)
{
    BOOST_ASSERT(0 <= id && id <= 360. / discrete_angle_step_size);
    return discrete_angle_step_size * id;
}

void BearingClass::resetContinuous(const double bearing) {
    const auto id = angleToDiscreteID(bearing);
    resetDiscreteID(id);
}

void BearingClass::resetDiscreteID(const std::uint8_t id) {
    available_bearings_mask &= ~(1<<id);
}

bool BearingClass::hasContinuous(const double bearing) const
{
    const auto id = angleToDiscreteID(bearing);
    return hasDiscrete(id);
}

bool BearingClass::hasDiscrete(const std::uint8_t id) const
{
    return 0 != (available_bearings_mask & (1<<id));
}

BearingClass::BearingClass() : available_bearings_mask(0) {}

bool BearingClass::operator==(const BearingClass &other) const
{
    return other.available_bearings_mask == available_bearings_mask;
}

bool BearingClass::operator<(const BearingClass &other) const
{
    return available_bearings_mask < other.available_bearings_mask;
}

bool BearingClass::addContinuous(const double angle)
{
    return addDiscreteID(angleToDiscreteID(angle));
}

bool BearingClass::addDiscreteID(const std::uint8_t discrete_id)
{
    const auto mask = (1 << discrete_id);
    const auto is_new = (0 == (available_bearings_mask & mask));
    available_bearings_mask |= mask;
    return is_new;
}

std::vector<double> BearingClass::getAvailableBearings() const
{
    std::vector<double> result;
    // account for some basic inaccuracries of double
    for (std::size_t discrete_id = 0; discrete_id * discrete_angle_step_size <= 361; ++discrete_id)
    {
        // ervery set bit indicates a bearing
        if (available_bearings_mask & (1 << discrete_id))
            result.push_back(discrete_id * discrete_angle_step_size);
    }
    return result;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
