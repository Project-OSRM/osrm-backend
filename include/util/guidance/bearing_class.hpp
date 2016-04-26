#ifndef OSRM_UTIL_GUIDANCE_BEARING_CLASS_HPP_
#define OSRM_UTIL_GUIDANCE_BEARING_CLASS_HPP_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace osrm
{
namespace util
{
namespace guidance
{
class BearingClass;
} // namespace guidance
} // namespace util
} // namespace osrm

namespace std
{
template <> struct hash<::osrm::util::guidance::BearingClass>
{
    inline std::size_t operator()(const ::osrm::util::guidance::BearingClass &bearing_class) const;
};
} // namespace std

namespace osrm
{
namespace util
{
namespace guidance
{

class BearingClass
{
  public:
    using FlagBaseType = std::uint32_t;
    const static constexpr double discrete_angle_step_size = 360. / 24;

    BearingClass();

    // add a continuous angle to the, returns true if no item existed that uses the same discrete
    // angle
    bool addContinuous(const double bearing);
    // add a discrete ID, returns true if no item existed that uses the same discrete angle
    bool addDiscreteID(const std::uint8_t id);

    // hashing
    bool operator==(const BearingClass &other) const;

    // sorting
    bool operator<(const BearingClass &other) const;

    std::vector<double> getAvailableBearings() const;

    // get a discrete representation of an angle. Required to map a bearing/angle to the discrete
    // ones stored within the class
    static std::uint8_t discreteBearingID(double angle);

    // we are hiding the access to the flags behind a protection wall, to make sure the bit logic
    // isn't tempered with
  private:
    // given a list of possible discrete angles, the available angles flag indicates the presence of
    // a given turn at the intersection
    FlagBaseType available_bearings_mask;

    // allow hash access to internal representation
    friend std::size_t std::hash<BearingClass>::operator()(const BearingClass &) const;
};

} // namespace guidance
} // namespace util
} // namespace osrm

// make Bearing Class hasbable
namespace std
{
inline size_t hash<::osrm::util::guidance::BearingClass>::
operator()(const ::osrm::util::guidance::BearingClass &bearing_class) const
{
    return hash<::osrm::util::guidance::BearingClass::FlagBaseType>()(
        bearing_class.available_bearings_mask);
}
} // namespace std

#endif /* OSRM_UTIL_GUIDANCE_BEARING_CLASS_HPP_ */
