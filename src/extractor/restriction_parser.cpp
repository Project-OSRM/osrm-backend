#include "extractor/restriction_parser.hpp"
#include "extractor/extraction_way.hpp"

#include "extractor/external_memory_node.hpp"
#include "util/lua_util.hpp"
#include "util/osrm_exception.hpp"
#include "util/simple_logger.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional/optional.hpp>
#include <boost/ref.hpp>
#include <boost/regex.hpp>

#include <osmium/osm.hpp>
#include <osmium/tags/regex_filter.hpp>

#include <algorithm>
#include <iterator>

namespace
{
int lua_error_callback(lua_State *lua_state)
{
    std::string error_msg = lua_tostring(lua_state, -1);
    throw osrm::exception("ERROR occurred in profile script:\n" + error_msg);
}
}

RestrictionParser::RestrictionParser(lua_State *lua_state) : use_turn_restrictions(true)
{
    ReadUseRestrictionsSetting(lua_state);

    if (use_turn_restrictions)
    {
        ReadRestrictionExceptions(lua_state);
    }
}

void RestrictionParser::ReadUseRestrictionsSetting(lua_State *lua_state)
{
    if (0 == luaL_dostring(lua_state, "return use_turn_restrictions\n") &&
        lua_isboolean(lua_state, -1))
    {
        use_turn_restrictions = lua_toboolean(lua_state, -1);
    }

    if (use_turn_restrictions)
    {
        SimpleLogger().Write() << "Using turn restrictions";
    }
    else
    {
        SimpleLogger().Write() << "Ignoring turn restrictions";
    }
}

void RestrictionParser::ReadRestrictionExceptions(lua_State *lua_state)
{
    if (lua_function_exists(lua_state, "get_exceptions"))
    {
        luabind::set_pcall_callback(&lua_error_callback);
        // get list of turn restriction exceptions
        luabind::call_function<void>(lua_state, "get_exceptions",
                                     boost::ref(restriction_exceptions));
        const unsigned exception_count = restriction_exceptions.size();
        SimpleLogger().Write() << "Found " << exception_count
                               << " exceptions to turn restrictions:";
        for (const std::string &str : restriction_exceptions)
        {
            SimpleLogger().Write() << "  " << str;
        }
    }
    else
    {
        SimpleLogger().Write() << "Found no exceptions to turn restrictions";
    }
}

/**
 * Tries to parse an relation as turn restriction. This can fail for a number of
 * reasons, this the return type is a boost::optional<T>.
 *
 * Some restrictions can also be ignored: See the ```get_exceptions``` function
 * in the corresponding profile.
 */
boost::optional<InputRestrictionContainer>
RestrictionParser::TryParse(const osmium::Relation &relation) const
{
    // return if turn restrictions should be ignored
    if (!use_turn_restrictions)
    {
        return {};
    }

    osmium::tags::KeyPrefixFilter filter(false);
    filter.add(true, "restriction");

    const osmium::TagList &tag_list = relation.tags();

    osmium::tags::KeyPrefixFilter::iterator fi_begin(filter, tag_list.begin(), tag_list.end());
    osmium::tags::KeyPrefixFilter::iterator fi_end(filter, tag_list.end(), tag_list.end());

    // if it's a restriction, continue;
    if (std::distance(fi_begin, fi_end) == 0)
    {
        return {};
    }

    // check if the restriction should be ignored
    const char *except = relation.get_value_by_key("except");
    if (except != nullptr && ShouldIgnoreRestriction(except))
    {
        return {};
    }

    bool is_only_restriction = false;

    for (; fi_begin != fi_end; ++fi_begin)
    {
        const std::string key(fi_begin->key());
        const std::string value(fi_begin->value());

        if (value.find("only_") == 0)
        {
            is_only_restriction = true;
        }

        // if the "restriction*" key is longer than 11 chars, it is a conditional exception (i.e.
        // "restriction:<transportation_type>")
        if (key.size() > 11)
        {
            const auto ex_suffix = [&](const std::string &exception)
            {
                return boost::algorithm::ends_with(key, exception);
            };
            bool is_actually_restricted =
                std::any_of(begin(restriction_exceptions), end(restriction_exceptions), ex_suffix);

            if (!is_actually_restricted)
            {
                return {};
            }
        }
    }

    InputRestrictionContainer restriction_container(is_only_restriction);

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
            // Make sure nodes appear only in the role if a via node
            if (0 == strcmp("from", role) || 0 == strcmp("to", role))
            {
                continue;
            }
            BOOST_ASSERT(0 == strcmp("via", role));

            // set via node id
            restriction_container.restriction.via.node = member.ref();
            break;

        case osmium::item_type::way:
            BOOST_ASSERT(0 == strcmp("from", role) || 0 == strcmp("to", role) ||
                         0 == strcmp("via", role));
            if (0 == strcmp("from", role))
            {
                restriction_container.restriction.from.way = member.ref();
            }
            else if (0 == strcmp("to", role))
            {
                restriction_container.restriction.to.way = member.ref();
            }
            // else if (0 == strcmp("via", role))
            // {
            //     not yet suppported
            //     restriction_container.restriction.via.way = member.ref();
            // }
            break;
        case osmium::item_type::relation:
            // not yet supported, but who knows what the future holds...
            break;
        default:
            // shouldn't ever happen
            break;
        }
    }
    return boost::make_optional(std::move(restriction_container));
}

bool RestrictionParser::ShouldIgnoreRestriction(const std::string &except_tag_string) const
{
    // should this restriction be ignored? yes if there's an overlap between:
    // a) the list of modes in the except tag of the restriction
    //    (except_tag_string), eg: except=bus;bicycle
    // b) the lua profile defines a hierachy of modes,
    //    eg: [access, vehicle, bicycle]

    if (except_tag_string.empty())
    {
        return false;
    }

    // Be warned, this is quadratic work here, but we assume that
    // only a few exceptions are actually defined.
    std::vector<std::string> exceptions;
    boost::algorithm::split_regex(exceptions, except_tag_string, boost::regex("[;][ ]*"));

    return std::any_of(std::begin(exceptions), std::end(exceptions),
                       [&](const std::string &current_string)
                       {
                           return std::end(restriction_exceptions) !=
                               std::find(std::begin(restriction_exceptions),
                                         std::end(restriction_exceptions), current_string);
                       });
}
