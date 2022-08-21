#ifndef OSRM_UTIL_GUIDANCE_BEARING_CLASS_HPP_
#define OSRM_UTIL_GUIDANCE_BEARING_CLASS_HPP_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include <boost/functional/hash.hpp>

#include "util/typedefs.hpp"

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
    BearingClass();

    // Add a bearing to the set
    void add(const DiscreteBearing bearing);

    // hashing
    bool operator==(const BearingClass &other) const;

    // sorting
    bool operator<(const BearingClass &other) const;

    const std::vector<DiscreteBearing> &getAvailableBearings() const;

    std::size_t findMatchingBearing(const double bearing) const;

    const constexpr static double discrete_step_size = 360. / 24.;
    static DiscreteBearing getDiscreteBearing(const double bearing);

  private:
    std::vector<DiscreteBearing> available_bearings;

    // allow hash access to internal representation
    friend std::size_t std::hash<BearingClass>::operator()(const BearingClass &) const;
};

} // namespace guidance
} // namespace util
} // namespace osrm

// make Bearing Class hasbable
namespace std
{
inline size_t hash<::osrm::util::guidance::BearingClass>::operator()(
    const ::osrm::util::guidance::BearingClass &bearing_class) const
{
    return boost::hash_value(bearing_class.available_bearings);
}
} // namespace std

#endif /* OSRM_UTIL_GUIDANCE_BEARING_CLASS_HPP_ */
