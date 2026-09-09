#include "guidance/turn_classification.hpp"

#include "util/to_osm_link.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <cstddef>

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
    // Only an intersection that cannot be discretized can carry more roads than the
    // entry class holds: a meshed plaza vertex, where every line of sight is a way.
    // Counted rather than reported one road at a time, since such a vertex is
    // classified once per edge arriving at it, which made this the loudest thing in an
    // extraction log by two orders of magnitude -- 157 094 lines over Ile-de-France,
    // from 276 vertices.
    std::size_t unrecorded = 0;
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
                // a discretizable intersection has a road per bearing bucket at most,
                // 24, which is within the capacity
                const bool recorded = entry_class.activate(number);
                BOOST_ASSERT(recorded);
                (void)recorded;
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
                    ++unrecorded;
                }
            }
            bearing_class.add(std::round(road.perceived_bearing));
            ++number;
        }
    }
    if (unrecorded > 0)
    {
        util::Log(logWARNING) << unrecorded << " roads allowing entry could not be recorded at "
                              << "an intersection of " << number << " roads (capacity "
                              << util::guidance::EntryClass::CAPACITY << ") at "
                              << util::toOSMLink(location);
    }
    return std::make_pair(entry_class, bearing_class);
}

} // namespace osrm::guidance
