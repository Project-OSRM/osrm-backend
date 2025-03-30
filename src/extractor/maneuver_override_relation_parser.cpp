#include "extractor/maneuver_override_relation_parser.hpp"
#include "extractor/maneuver_override.hpp"

#include <boost/ref.hpp>

#include <optional>
#include <osmium/osm.hpp>
#include <osmium/tags/filter.hpp>
#include <osmium/tags/taglist.hpp>

#include <algorithm>
#include <iterator>

namespace osrm::extractor
{

ManeuverOverrideRelationParser::ManeuverOverrideRelationParser() {}

/**
 * Parses the `type=maneuver` relation.  Reads the fields, and puts data
 * into an InputManeuverOverride object, if the relation is considered
 * valid (i.e. has the minimum tags we expect).
 */
std::optional<InputManeuverOverride>
ManeuverOverrideRelationParser::TryParse(const osmium::Relation &relation) const
{

    // Support both American and British spellings of maneuver/manoeuvre
    osmium::tags::KeyValueFilter filter{false};
    filter.add(true, "type", "maneuver");
    filter.add(true, "type", "manoeuvre");

    const osmium::TagList &tag_list = relation.tags();

    if (osmium::tags::match_none_of(tag_list, filter))
    // if it's not a maneuver, continue;
    {
        return std::nullopt;
    }

    // we pretend every restriction is a conditional restriction. If we do not find any restriction,
    // we can trim away the vector after parsing
    InputManeuverOverride maneuver_override;

    // Handle both spellings
    if (relation.tags().has_key("manoeuvre"))
    {
        maneuver_override.maneuver = relation.tags().get_value_by_key("manoeuvre", "");
    }
    else
    {
        maneuver_override.maneuver = relation.tags().get_value_by_key("maneuver", "");
    }

    maneuver_override.direction = relation.tags().get_value_by_key("direction", "");

    bool valid_relation = true;
    OSMNodeID via_node = SPECIAL_OSM_NODEID;
    OSMWayID from = SPECIAL_OSM_WAYID, to = SPECIAL_OSM_WAYID;
    std::vector<OSMWayID> via_ways;

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
            // set via node id
            valid_relation &= via_node == SPECIAL_OSM_NODEID;
            via_node = OSMNodeID{static_cast<std::uint64_t>(member.ref())};
            break;
        }
        case osmium::item_type::way:
            BOOST_ASSERT(0 == strcmp("from", role) || 0 == strcmp("to", role) ||
                         0 == strcmp("via", role));
            if (0 == strcmp("from", role))
            {
                valid_relation &= from == SPECIAL_OSM_WAYID;
                from = OSMWayID{static_cast<std::uint64_t>(member.ref())};
            }
            else if (0 == strcmp("to", role))
            {
                valid_relation &= to == SPECIAL_OSM_WAYID;
                to = OSMWayID{static_cast<std::uint64_t>(member.ref())};
            }
            else if (0 == strcmp("via", role))
            {
                via_ways.push_back(OSMWayID{static_cast<std::uint64_t>(member.ref())});
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

    // Check required roles
    valid_relation &= from != SPECIAL_OSM_WAYID;
    valid_relation &= to != SPECIAL_OSM_WAYID;
    valid_relation &= via_node != SPECIAL_OSM_NODEID;

    if (valid_relation)
    {
        if (via_ways.empty())
        {
            maneuver_override.turn_path.node_or_way = InputViaNodePath{{from}, {via_node}, {to}};
        }
        else
        {
            maneuver_override.turn_path.node_or_way =
                InputViaWayPath{{from}, std::move(via_ways), {to}};
        }
        maneuver_override.via_node = via_node;
    }
    else
    {
        return std::nullopt;
    }
    return maneuver_override;
}
} // namespace osrm::extractor
