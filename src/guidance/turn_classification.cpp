#include "guidance/turn_classification.hpp"

#include "util/to_osm_link.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace osrm::guidance
{

std::pair<util::guidance::EntryClass, util::guidance::BearingClass>
classifyIntersection(Intersection intersection, const osrm::util::Coordinate &location)
{
    if (intersection.empty())
        return {};

    std::sort(intersection.begin(),
              intersection.end(),
              [](const ConnectedRoad &left, const ConnectedRoad &right)
              { return left.perceived_bearing < right.perceived_bearing; });

    util::guidance::EntryClass entry_class;
    util::guidance::BearingClass bearing_class;

    const bool canBeDiscretized = [&]()
    {
        if (intersection.size() <= 1)
            return true;

        DiscreteBearing last_discrete_bearing = util::guidance::BearingClass::getDiscreteBearing(
            std::round(intersection.back().perceived_bearing));
        for (const auto &road : intersection)
        {
            const DiscreteBearing discrete_bearing =
                util::guidance::BearingClass::getDiscreteBearing(
                    std::round(road.perceived_bearing));
            if (discrete_bearing == last_discrete_bearing)
                return false;
            last_discrete_bearing = discrete_bearing;
        }
        return true;
    }();

    // finally transfer data to the entry/bearing classes
    std::size_t number = 0;
    if (canBeDiscretized)
    {
        if (util::guidance::BearingClass::getDiscreteBearing(
                intersection.back().perceived_bearing) <
            util::guidance::BearingClass::getDiscreteBearing(
                intersection.front().perceived_bearing))
        {
            intersection.insert(intersection.begin(), intersection.back());
            intersection.pop_back();
        }
        for (const auto &road : intersection)
        {
            if (road.entry_allowed)
            {
                if (!entry_class.activate(number))
                {
                    util::Log(logWARNING) << "Road " << number << " was not activated at "
                                          << util::toOSMLink(location);
                }
            }

            auto discrete_bearing_class = util::guidance::BearingClass::getDiscreteBearing(
                std::round(road.perceived_bearing));
            bearing_class.add(std::round(discrete_bearing_class *
                                         util::guidance::BearingClass::discrete_step_size));
            ++number;
        }
    }
    else
    {
        for (const auto &road : intersection)
        {
            if (road.entry_allowed)
            {
                if (!entry_class.activate(number))
                {
                    util::Log(logWARNING) << "Road " << number << " was not activated at "
                                          << util::toOSMLink(location);
                }
            }
            bearing_class.add(std::round(road.perceived_bearing));
            ++number;
        }
    }
    return std::make_pair(entry_class, bearing_class);
}

} // namespace osrm::guidance
