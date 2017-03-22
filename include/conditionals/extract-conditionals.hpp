#ifndef EXTRACT_CONDITIONALS_HPP
#define EXTRACT_CONDITIONALS_HPP

#include "util/conditional_restrictions.hpp"
#include "util/for_each_pair.hpp"
#include "util/log.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
#include <boost/program_options.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

#include <osmium/handler.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/tags/regex_filter.hpp>
#include <osmium/visitor.hpp>

#include <fstream>
#include <unordered_set>

// Data types and functions for conditional restriction
struct ConditionalRestriction
{
    osmium::object_id_type from;
    osmium::object_id_type via;
    osmium::object_id_type to;
    std::string tag;
    std::string value;
    std::string condition;
};

struct LocatedConditionalRestriction
{
    osmium::Location location;
    ConditionalRestriction restriction;
};

// Data types and functions for conditional speed limits
struct ConditionalSpeedLimit
{
    osmium::object_id_type from;
    osmium::object_id_type to;
    std::string tag;
    int value;
    std::string condition;
};

struct LocatedConditionalSpeedLimit
{
    osmium::Location location;
    ConditionalSpeedLimit speed_limit;
};

auto ParseGlobalArguments(int argc, char *argv[]);
void ParseDumpCommandArguments(const char *executable,
                               const char *command_name,
                               const std::vector<std::string> &arguments,
                               std::string &osm_filename,
                               std::string &csv_filename);
void ParseCheckCommandArguments(const char *executable,
                                const char *command_name,
                                const std::vector<std::string> &arguments,
                                std::string &input_filename,
                                std::string &output_filename,
                                std::string &tz_filename,
                                std::time_t &utc_time,
                                std::int64_t &restriction_value);
int RestrictionsDumpCommand(const char *executable, const std::vector<std::string> &arguments);
int SpeedLimitsDumpCommand(const char *executable, const std::vector<std::string> &arguments);
int RestrictionsCheckCommand(const char *executable, const std::vector<std::string> &arguments);
int SpeedLimitsCheckCommand(const char *executable, const std::vector<std::string> &arguments);

// The first pass relations handler that collects conditional restrictions
class ConditionalRestrictionsCollector : public osmium::handler::Handler
{
  public:
    ConditionalRestrictionsCollector(std::vector<ConditionalRestriction> &restrictions)
        : restrictions(restrictions)
    {
        tag_filter.add(true, std::regex("^restriction.*:conditional$"));
    }

    void relation(const osmium::Relation &relation) const
    {
        // Check if relation contains any ":conditional" tag
        const osmium::TagList &tags = relation.tags();
        typename decltype(tag_filter)::iterator first(tag_filter, tags.begin(), tags.end());
        typename decltype(tag_filter)::iterator last(tag_filter, tags.end(), tags.end());
        if (first == last)
            return;

        // Get member references of from(way) -> via(node) -> to(way)
        auto from = invalid_id, via = invalid_id, to = invalid_id;
        for (const auto &member : relation.members())
        {
            if (member.ref() == 0)
                continue;

            if (member.type() == osmium::item_type::node && strcmp(member.role(), "via") == 0)
            {
                via = member.ref();
            }
            else if (member.type() == osmium::item_type::way && strcmp(member.role(), "from") == 0)
            {
                from = member.ref();
            }
            else if (member.type() == osmium::item_type::way && strcmp(member.role(), "to") == 0)
            {
                to = member.ref();
            }
        }

        if (from == invalid_id || via == invalid_id || to == invalid_id)
            return;

        for (; first != last; ++first)
        {
            // Parse condition and add independent value/condition pairs
            const auto &parsed = osrm::util::ParseConditionalRestrictions(first->value());

            if (parsed.empty())
            {
                osrm::util::Log(logWARNING) << "Conditional restriction parsing failed for \""
                                            << first->value() << "\" at the turn " << from << " -> "
                                            << via << " -> " << to;
                continue;
            }

            for (auto &restriction : parsed)
            {
                restrictions.push_back(
                    {from, via, to, first->key(), restriction.value, restriction.condition});
            }
        }
    }

  private:
    const osmium::object_id_type invalid_id = std::numeric_limits<osmium::object_id_type>::max();
    osmium::tags::Filter<std::regex> tag_filter;
    std::vector<ConditionalRestriction> &restrictions;
};

// The second pass handler that collects related nodes and ways.
// process_restrictions method calls for every collected conditional restriction
// a callback function with the prototype:
//    (location, from node, via node, to node, tag, value, condition)
// If the restriction value starts with "only_" the callback function will be called
// for every edge adjacent to `via` node but not containing `to` node.
class ConditionalRestrictionsHandler : public osmium::handler::Handler
{

  public:
    ConditionalRestrictionsHandler(const std::vector<ConditionalRestriction> &restrictions)
        : restrictions(restrictions)
    {
        for (auto &restriction : restrictions)
            related_vias.insert(restriction.via);

        via_adjacency.reserve(restrictions.size() * 2);
    }

    void node(const osmium::Node &node)
    {
        const osmium::object_id_type id = node.id();
        if (related_vias.find(id) != related_vias.end())
        {
            location_storage.set(static_cast<osmium::unsigned_object_id_type>(id), node.location());
        }
    }

    void way(const osmium::Way &way)
    {
        const auto &nodes = way.nodes();

        if (related_vias.find(nodes.front().ref()) != related_vias.end())
            via_adjacency.push_back(std::make_tuple(nodes.front().ref(), way.id(), nodes[1].ref()));

        if (related_vias.find(nodes.back().ref()) != related_vias.end())
            via_adjacency.push_back(
                std::make_tuple(nodes.back().ref(), way.id(), nodes[nodes.size() - 2].ref()));
    }

    template <typename Callback> void process(Callback callback)
    {
        location_storage.sort();
        std::sort(via_adjacency.begin(), via_adjacency.end());

        auto adjacent_nodes = [this](auto node) {
            auto first = std::lower_bound(
                via_adjacency.begin(), via_adjacency.end(), adjacency_type{node, 0, 0});
            auto last =
                std::upper_bound(first, via_adjacency.end(), adjacency_type{node + 1, 0, 0});
            return std::make_pair(first, last);
        };

        auto find_node = [](auto nodes, auto way) {
            return std::find_if(
                nodes.first, nodes.second, [way](const auto &n) { return std::get<1>(n) == way; });
        };

        for (auto &restriction : restrictions)
        {
            auto nodes = adjacent_nodes(restriction.via);
            auto from = find_node(nodes, restriction.from);
            if (from == nodes.second)
                continue;

            const auto &location =
                location_storage.get(static_cast<osmium::unsigned_object_id_type>(restriction.via));

            if (boost::algorithm::starts_with(restriction.value, "only_"))
            {
                for (auto it = nodes.first; it != nodes.second; ++it)
                {
                    if (std::get<1>(*it) == restriction.to)
                        continue;

                    callback(location,
                             std::get<2>(*from),
                             restriction.via,
                             std::get<2>(*it),
                             restriction.tag,
                             restriction.value,
                             restriction.condition);
                }
            }
            else
            {
                auto to = find_node(nodes, restriction.to);
                if (to != nodes.second)
                {
                    callback(location,
                             std::get<2>(*from),
                             restriction.via,
                             std::get<2>(*to),
                             restriction.tag,
                             restriction.value,
                             restriction.condition);
                }
            }
        }
    }

  private:
    using index_type =
        osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
    using adjacency_type =
        std::tuple<osmium::object_id_type, osmium::object_id_type, osmium::object_id_type>;

    const std::vector<ConditionalRestriction> &restrictions;
    std::unordered_set<osmium::object_id_type> related_vias;
    std::vector<adjacency_type> via_adjacency;
    index_type location_storage;
};

class ConditionalSpeedLimitsCollector : public osmium::handler::Handler
{

  public:
    ConditionalSpeedLimitsCollector()
    {
        tag_filter.add(true, std::regex("^maxspeed.*:conditional$"));
    }

    void node(const osmium::Node &node)
    {
        const osmium::object_id_type id = node.id();
        if (related_nodes.find(id) != related_nodes.end())
        {
            location_storage.set(static_cast<osmium::unsigned_object_id_type>(id), node.location());
        }
    }

    void way(const osmium::Way &way)
    {
        const osmium::TagList &tags = way.tags();
        typename decltype(tag_filter)::iterator first(tag_filter, tags.begin(), tags.end());
        typename decltype(tag_filter)::iterator last(tag_filter, tags.end(), tags.end());
        if (first == last)
            return;

        for (; first != last; ++first)
        {
            // Parse condition and add independent value/condition pairs
            const auto &parsed = osrm::util::ParseConditionalRestrictions(first->value());

            if (parsed.empty())
            {
                osrm::util::Log(logWARNING) << "Conditional speed limit parsing failed for \""
                                            << first->value() << "\" on the way " << way.id();
                continue;
            }

            // Collect node IDs for the second pass over nodes
            std::transform(way.nodes().begin(),
                           way.nodes().end(),
                           std::inserter(related_nodes, related_nodes.end()),
                           [](const auto &node_ref) { return node_ref.ref(); });

            // Collect speed limits
            for (auto &speed_limit : parsed)
            {
                // Convert value to an integer
                int speed_limit_value;
                {
                    namespace qi = boost::spirit::qi;
                    std::string::const_iterator first(speed_limit.value.begin()),
                        last(speed_limit.value.end());
                    if (!qi::parse(first, last, qi::int_, speed_limit_value) || first != last)
                    {
                        osrm::util::Log(logWARNING)
                            << "Conditional speed limit has non-integer value \""
                            << speed_limit.value << "\" on the way " << way.id();
                        continue;
                    }
                }

                std::string key = first->key();
                const bool is_forward = key.find(":forward:") != std::string::npos;
                const bool is_backward = key.find(":backward:") != std::string::npos;
                const bool is_direction_defined = is_forward || is_backward;

                if (is_forward || !is_direction_defined)
                {
                    osrm::util::for_each_pair(way.nodes().cbegin(),
                                              way.nodes().cend(),
                                              [&](const auto &from, const auto &to) {
                                                  speed_limits.push_back(
                                                      ConditionalSpeedLimit{from.ref(),
                                                                            to.ref(),
                                                                            key,
                                                                            speed_limit_value,
                                                                            speed_limit.condition});
                                              });
                }

                if (is_backward || !is_direction_defined)
                {
                    osrm::util::for_each_pair(way.nodes().cbegin(),
                                              way.nodes().cend(),
                                              [&](const auto &to, const auto &from) {
                                                  speed_limits.push_back(
                                                      ConditionalSpeedLimit{from.ref(),
                                                                            to.ref(),
                                                                            key,
                                                                            speed_limit_value,
                                                                            speed_limit.condition});
                                              });
                }
            }
        }
    }

    template <typename Callback> void process(Callback callback)
    {
        location_storage.sort();

        for (auto &speed_limit : speed_limits)
        {
            const auto &location_from = location_storage.get(
                static_cast<osmium::unsigned_object_id_type>(speed_limit.from));
            const auto &location_to =
                location_storage.get(static_cast<osmium::unsigned_object_id_type>(speed_limit.to));
            osmium::Location location{(location_from.lon() + location_to.lon()) / 2,
                                      (location_from.lat() + location_to.lat()) / 2};

            callback(location,
                     speed_limit.from,
                     speed_limit.to,
                     speed_limit.tag,
                     speed_limit.value,
                     speed_limit.condition);
        }
    }

  private:
    using index_type =
        osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

    osmium::tags::Filter<std::regex> tag_filter;
    std::unordered_set<osmium::object_id_type> related_nodes;
    std::vector<ConditionalSpeedLimit> speed_limits;
    index_type location_storage;
};

#endif EXTRACT_CONDITIONALS_HPP
