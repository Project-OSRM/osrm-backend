#ifndef OSRM_INCLUDE_GUIDANCE_TURN_BEARING_HPP_
#define OSRM_INCLUDE_GUIDANCE_TURN_BEARING_HPP_

#include <cstdint>

#include <boost/assert.hpp>

namespace osrm
{
namespace guidance
{
namespace
{
const double bearing_scale = 360.0 / 256.0;
}

#pragma pack(push, 1)
class TurnBearing
{
  public:
    // discretizes a bearing into distinct units of 1.4 degrees
    TurnBearing(const double value = 0) : bearing(value / bearing_scale)
    {
        BOOST_ASSERT_MSG(value >= 0 && value < 360.0,
                         "Bearing value needs to be between 0 and 360 (exclusive)");
    }

    double Get() const { return bearing * bearing_scale; }

  private:
    std::uint8_t bearing;
};
#pragma pack(pop)

} // namespace guidance
} // namespace osrm

#endif /* OSRM_INCLUDE_GUIDANCE_TURN_BEARING_HPP_ */
