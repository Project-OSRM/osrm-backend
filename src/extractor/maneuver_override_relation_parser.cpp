#include "extractor/maneuver_override_relation_parser.hpp"
#include "extractor/maneuver_override.hpp"

#include "util/log.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/optional/optional.hpp>
#include <boost/ref.hpp>
#include <boost/regex.hpp>

#include <osmium/osm.hpp>
#include <osmium/tags/regex_filter.hpp>

#include <algorithm>
#include <iterator>

namespace osrm
{
namespace extractor
{

ManeuverOverrideRelationParser::ManeuverOverrideRelationParser() {}

/**
 * Parses the `type=maneuver` relation.  Reads the fields, and puts data
 * into an InputManeuverOverride object, if the relation is considered
 * valid (i.e. has the minimum tags we expect).
 */
boost::optional<InputManeuverOverride>
ManeuverOverrideRelationParser::TryParse(const osmium::Relation &relation) const
{
    osmium::tags::KeyFilter filter(false);
    filter.add(true, "maneuver");

    const osmium::TagList &tag_list = relation.tags();

    osmium::tags::KeyFilter::iterator fi_begin(filter, tag_list.begin(), tag_list.end());
    osmium::tags::KeyFilter::iterator fi_end(filter, tag_list.end(), tag_list.end());

    // if it's not a maneuver, continue;
    if (std::distance(fi_begin, fi_end) == 0)
    {
        return boost::none;
    }

    // we pretend every restriction is a conditional restriction. If we do not find any restriction,
    // we can trim away the vector after parsing
    InputManeuverOverride maneuver_override;

    maneuver_override.maneuver = relation.tags().get_value_by_key("maneuver", "");
    maneuver_override.direction = relation.tags().get_value_by_key("direction", "");

    boost::optional<std::uint64_t> from = boost::none, via = boost::none, to = boost::none;
    std::vector<std::uint64_t> via_ways;

    for (const auto &member : relation.members())
    {
        const char *role = member.role();
        if (strcmp("from", role) != 0 && strcmp("to", role) != 0 && strcmp("via", role) != 0)
        {
            continue;
        }

        switch (member.type())
        {
        case osmium::item_type::node:
        {

            // Make sure nodes appear only in the role if a via node
            if (0 == strcmp("from", role) || 0 == strcmp("to", role))
            {
                continue;
            }
            BOOST_ASSERT(0 == strcmp("via", role));
            via = static_cast<std::uint64_t>(member.ref());
            // set via node id
            break;
        }
        case osmium::item_type::way:
            BOOST_ASSERT(0 == strcmp("from", role) || 0 == strcmp("to", role) ||
                         0 == strcmp("via", role));
            if (0 == strcmp("from", role))
            {
                from = static_cast<std::uint64_t>(member.ref());
            }
            else if (0 == strcmp("to", role))
            {
                to = static_cast<std::uint64_t>(member.ref());
            }
            else if (0 == strcmp("via", role))
            {
                via_ways.push_back(static_cast<std::uint64_t>(member.ref()));
            }
            break;
        case osmium::item_type::relation:
            // not yet supported, but who knows what the future holds...
            break;
        default:
            // shouldn't ever happen
            break;
        }
    }

    if (from && (via || via_ways.size() > 0) && to)
    {
        via_ways.insert(via_ways.begin(), *from);
        via_ways.push_back(*to);
        if (via)
        {
            maneuver_override.via_node = {*via};
        }
        for (const auto &n : via_ways)
        {
            maneuver_override.via_ways.push_back(OSMWayID{n});
        }
    }
    else
    {
        return boost::none;
    }
    return maneuver_override;
}
}
}
