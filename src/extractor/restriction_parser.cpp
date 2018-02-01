#include "extractor/restriction_parser.hpp"
#include "extractor/profile_properties.hpp"

#include "util/conditional_restrictions.hpp"
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

RestrictionParser::RestrictionParser(bool use_turn_restrictions_,
                                     bool parse_conditionals_,
                                     std::vector<std::string> &restrictions_)
    : use_turn_restrictions(use_turn_restrictions_), parse_conditionals(parse_conditionals_),
      restrictions(restrictions_)
{
    if (use_turn_restrictions)
    {
        const unsigned count = restrictions.size();
        if (count > 0)
        {
            util::Log() << "Found " << count << " turn restriction tags:";
            for (const std::string &str : restrictions)
            {
                util::Log() << "  " << str;
            }
        }
        else
        {
            util::Log() << "Found no turn restriction tags";
        }
    }
}

/**
 * Tries to parse a relation as a turn restriction. This can fail for a number of
 * reasons. The return type is a boost::optional<T>.
 *
 * Some restrictions can also be ignored: See the ```get_restrictions``` function
 * in the corresponding profile. We use it for both namespacing restrictions, as in
 * restriction:motorcar as well as whitelisting if its in except:motorcar.
 */
boost::optional<InputConditionalTurnRestriction>
RestrictionParser::TryParse(const osmium::Relation &relation) const
{
    // return if turn restrictions should be ignored
    if (!use_turn_restrictions)
    {
        return boost::none;
    }

    osmium::tags::KeyFilter filter(false);
    filter.add(true, "restriction");
    if (parse_conditionals)
    {
        filter.add(true, "restriction:conditional");
        for (const auto &namespaced : restrictions)
        {
            filter.add(true, "restriction:" + namespaced + ":conditional");
        }
    }

    // Not only use restriction= but also e.g. restriction:motorcar=
    // Include restriction:{mode}:conditional if flagged
    for (const auto &namespaced : restrictions)
    {
        filter.add(true, "restriction:" + namespaced);
    }

    const osmium::TagList &tag_list = relation.tags();

    osmium::tags::KeyFilter::iterator fi_begin(filter, tag_list.begin(), tag_list.end());
    osmium::tags::KeyFilter::iterator fi_end(filter, tag_list.end(), tag_list.end());

    // if it's not a restriction, continue;
    if (std::distance(fi_begin, fi_end) == 0)
    {
        return boost::none;
    }

    // check if the restriction should be ignored
    const char *except = relation.get_value_by_key("except");
    if (except != nullptr && ShouldIgnoreRestriction(except))
    {
        return boost::none;
    }

    bool is_only_restriction = false;

    for (; fi_begin != fi_end; ++fi_begin)
    {
        const std::string key(fi_begin->key());
        const std::string value(fi_begin->value());

        // documented OSM restriction tags start either with only_* or no_*;
        // check and return on these values, and ignore no_*_on_red or unrecognized values
        if (value.find("only_") == 0)
        {
            is_only_restriction = true;
        }
        else if (value.find("no_") == 0 && !boost::algorithm::ends_with(value, "_on_red"))
        {
            is_only_restriction = false;
        }
        else // unrecognized value type
        {
            return boost::none;
        }
    }

    // we pretend every restriction is a conditional restriction. If we do not find any restriction,
    // we can trim away the vector after parsing
    InputConditionalTurnRestriction restriction_container;
    restriction_container.is_only = is_only_restriction;

    constexpr auto INVALID_OSM_ID = std::numeric_limits<std::uint64_t>::max();
    auto from = INVALID_OSM_ID;
    auto via = INVALID_OSM_ID;
    auto to = INVALID_OSM_ID;
    bool is_node_restriction = true;

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
            is_node_restriction = true;
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
                via = static_cast<std::uint64_t>(member.ref());
                is_node_restriction = false;
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

    // parse conditional tags
    if (parse_conditionals)
    {
        osmium::tags::KeyFilter::iterator fi_begin(filter, tag_list.begin(), tag_list.end());
        osmium::tags::KeyFilter::iterator fi_end(filter, tag_list.end(), tag_list.end());
        for (; fi_begin != fi_end; ++fi_begin)
        {
            const std::string key(fi_begin->key());
            const std::string value(fi_begin->value());

            // Parse condition and add independent value/condition pairs
            const auto &parsed = osrm::util::ParseConditionalRestrictions(value);

            if (parsed.empty())
                continue;

            for (const auto &p : parsed)
            {
                std::vector<util::OpeningHours> hours = util::ParseOpeningHours(p.condition);
                // found unrecognized condition, continue
                if (hours.empty())
                    return boost::none;

                restriction_container.condition = std::move(hours);
            }
        }
    }

    if (from != INVALID_OSM_ID && via != INVALID_OSM_ID && to != INVALID_OSM_ID)
    {
        if (is_node_restriction)
        {
            // template struct requires bracket for ID initialisation :(
            restriction_container.node_or_way = InputNodeRestriction{{from}, {via}, {to}};
        }
        else
        {
            // template struct requires bracket for ID initialisation :(
            restriction_container.node_or_way = InputWayRestriction{{from}, {via}, {to}};
        }
        return restriction_container;
    }
    else
    {
        return boost::none;
    }
}

bool RestrictionParser::ShouldIgnoreRestriction(const std::string &except_tag_string) const
{
    // should this restriction be ignored? yes if there's an overlap between:
    // a) the list of modes in the except tag of the restriction
    //    (except_tag_string), eg: except=bus;bicycle
    // b) the lua profile defines a hierarchy of modes,
    //    eg: [access, vehicle, bicycle]

    if (except_tag_string.empty())
    {
        return false;
    }

    // Be warned, this is quadratic work here, but we assume that
    // only a few exceptions are actually defined.
    std::vector<std::string> exceptions;
    boost::algorithm::split_regex(exceptions, except_tag_string, boost::regex("[;][ ]*"));

    return std::any_of(
        std::begin(exceptions), std::end(exceptions), [&](const std::string &current_string) {
            return std::end(restrictions) !=
                   std::find(std::begin(restrictions), std::end(restrictions), current_string);
        });
}
}
}
