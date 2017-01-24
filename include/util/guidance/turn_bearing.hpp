#ifndef OSRM_INCLUDE_UTIL_TURN_BEARING_HPP_
#define OSRM_INCLUDE_UTIL_TURN_BEARING_HPP_

#include <cstdint>

namespace osrm
{
namespace util
{
namespace guidance
{

#pragma pack(push, 1)
class TurnBearing
{
  public:
    TurnBearing(const double value = 0);

    double Get() const;

  private:
    std::uint8_t bearing;
};
#pragma pack(pop)

} // namespace guidance
} // namespace util
} // namespace osrm

#endif /* OSRM_INCLUDE_UTIL_TURN_BEARING_HPP_ */
