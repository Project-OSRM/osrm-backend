#include "util/conditional_restrictions.hpp"
#include "util/exception.hpp"
#include "util/for_each_pair.hpp"
#include "util/log.hpp"
#include "util/opening_hours.hpp"
#include "util/version.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/program_options.hpp>
#include <boost/scope_exit.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

#include <osmium/handler.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/tags/regex_filter.hpp>
#include <osmium/visitor.hpp>

#include <shapefil.h>

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <chrono>
#include <cstdlib>
//#include <ctime>

// Program arguments parsing functions
auto ParseGlobalArguments(int argc, char *argv[])
{
    namespace po = boost::program_options;

    po::options_description global_options("Global options");
    global_options.add_options()("version,v", "Show version")("help,h", "Show this help message");

    po::options_description commands("Commands");
    commands.add_options()("command", po::value<std::string>())(
        "subargs", po::value<std::vector<std::string>>());

    po::positional_options_description pos;
    pos.add("command", 1).add("subargs", -1);

    po::variables_map vm;
    std::vector<std::string> command_options;
    try
    {
        po::options_description cmdline_options;
        cmdline_options.add(global_options).add(commands);

        po::parsed_options parsed = po::command_line_parser(argc, argv)
                                        .options(cmdline_options)
                                        .positional(pos)
                                        .allow_unregistered()
                                        .run();

        // split parsed options
        auto command = std::find_if(parsed.options.begin(), parsed.options.end(), [](auto &o) {
            return o.string_key == "command";
        });
        for (auto it = command; it != parsed.options.end(); ++it)
        {
            std::copy(it->original_tokens.begin(),
                      it->original_tokens.end(),
                      back_inserter(command_options));
        }

        parsed.options.erase(command, parsed.options.end());
        po::store(parsed, vm);
    }
    catch (const po::error &e)
    {
        throw osrm::util::exception(e.what());
    }

    if (vm.count("version"))
    {
        std::cout << OSRM_VERSION << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    if (vm.count("help") || command_options.empty())
    {
        const auto *executable = argv[0];
        std::cout << boost::filesystem::path(executable).filename().string()
                  << " [<global options>] <command> [<args>]\n\n"
                     "Supported commands are:\n\n"
                     "   cond-dump      Save conditional restrictions in CSV format\n"
                     "   cond-check     Check conditional restrictions and save in CSV format\n"
                     "   speed-dump     Save conditional speed limits in CSV format\n"
                     "   speed-check    Check conditional speed limits and save in CSV format\n"
                  << "\n"
                  << global_options;
        std::exit(EXIT_SUCCESS);
    }

    return command_options;
}

void ParseDumpCommandArguments(const char *executable,
                               const char *command_name,
                               const std::vector<std::string> &arguments,
                               std::string &osm_filename,
                               std::string &csv_filename)
{
    namespace po = boost::program_options;

    po::options_description options(command_name);
    options.add_options()("help,h", "Show this help message")(
        "input,i",
        po::value<std::string>(&osm_filename),
        "OSM input file in .osm, .osm.bz2 or .osm.pbf format")(
        "output,o",
        po::value<std::string>(&csv_filename),
        "Output conditional restrictions file in CSV format");

    po::positional_options_description pos;
    pos.add("input", 1).add("output", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(arguments).options(options).positional(pos).run(), vm);

    if (vm.count("help"))
    {
        std::cout << boost::filesystem::path(executable).filename().string() << " " << command_name
                  << " [<args>]\n\n"
                  << options;
        std::exit(EXIT_SUCCESS);
    }
    po::notify(vm);
}

void ParseCheckCommandArguments(const char *executable,
                                const char *command_name,
                                const std::vector<std::string> &arguments,
                                std::string &input_filename,
                                std::string &output_filename,
                                std::string &tz_filename,
                                std::time_t &utc_time,
                                std::int64_t &restriction_value)
{
    namespace po = boost::program_options;

    po::options_description options(command_name);
    options.add_options()("help,h", "Show this help message")(
        "input,i",
        po::value<std::string>(&input_filename),
        "Input conditional restrictions file in CSV format")(
        "output,o",
        po::value<std::string>(&output_filename),
        "Output conditional restrictions file in CSV format")(
        "tz-shapes,s",
        po::value<std::string>(&tz_filename)->default_value(tz_filename),
        "Timezones shape file without extension")(
        "utc-time,t",
        po::value<std::time_t>(&utc_time)->default_value(utc_time),
        "UTC time-stamp [default=current UTC time]")(
        "value,v",
        po::value<std::int64_t>(&restriction_value)->default_value(restriction_value),
        "Restriction value for active time conditional restrictions");

    po::positional_options_description pos;
    pos.add("input", 1).add("output", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(arguments).options(options).positional(pos).run(), vm);

    if (vm.count("help"))
    {
        std::cout << boost::filesystem::path(executable).filename().string() << command_name
                  << " [<args>]\n\n"
                  << options;
        std::exit(EXIT_SUCCESS);
    }
    po::notify(vm);
}

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

// clang-format off
BOOST_FUSION_ADAPT_ADT(LocatedConditionalRestriction,
  (osmium::object_id_type, osmium::object_id_type, obj.restriction.from, obj.restriction.from = val)
  (osmium::object_id_type, osmium::object_id_type, obj.restriction.via, obj.restriction.via = val)
  (osmium::object_id_type, osmium::object_id_type, obj.restriction.to, obj.restriction.to = val)
  (std::string, std::string const &, obj.restriction.tag, obj.restriction.tag = val)
  (std::string, std::string const &, obj.restriction.value, obj.restriction.value = val)
  (std::string, std::string const &, obj.restriction.condition, obj.restriction.condition = val)
  (std::int32_t, std::int32_t, obj.location.lon(), obj.location.set_lon(val))
  (std::int32_t, std::int32_t, obj.location.lat(), obj.location.set_lat(val)))
// clang-format on

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

int RestrictionsDumpCommand(const char *executable, const std::vector<std::string> &arguments)
{
    std::string osm_filename, csv_filename;
    ParseDumpCommandArguments(executable, "cond-dump", arguments, osm_filename, csv_filename);

    // Read OSM input file
    const osmium::io::File input_file(osm_filename);

    // Read relations
    std::vector<ConditionalRestriction> conditional_restrictions_prior;
    ConditionalRestrictionsCollector restrictions_collector(conditional_restrictions_prior);
    osmium::io::Reader reader1(
        input_file, osmium::io::read_meta::no, osmium::osm_entity_bits::relation);
    osmium::apply(reader1, restrictions_collector);
    reader1.close();

    // Handle nodes and ways in relations
    ConditionalRestrictionsHandler restrictions_handler(conditional_restrictions_prior);
    osmium::io::Reader reader2(input_file,
                               osmium::io::read_meta::no,
                               osmium::osm_entity_bits::node | osmium::osm_entity_bits::way);
    osmium::apply(reader2, restrictions_handler);
    reader2.close();

    // Prepare output stream
    std::streambuf *buf = std::cout.rdbuf();
    std::ofstream of;
    if (!csv_filename.empty())
    {
        of.open(csv_filename, std::ios::binary);
        buf = of.rdbuf();
    }
    std::ostream stream(buf);

    // Process collected restrictions and print CSV output
    restrictions_handler.process([&stream](const osmium::Location &location,
                                           osmium::object_id_type from,
                                           osmium::object_id_type via,
                                           osmium::object_id_type to,
                                           const std::string &tag,
                                           const std::string &value,
                                           const std::string &condition) {
        stream << from << "," << via << "," << to << "," << tag << "," << value << ",\""
               << condition << "\""
               << "," << std::setprecision(6) << location.lon() << "," << std::setprecision(6)
               << location.lat() << "\n";
    });

    return EXIT_SUCCESS;
}

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

// clang-format off
BOOST_FUSION_ADAPT_ADT(LocatedConditionalSpeedLimit,
  (osmium::object_id_type, osmium::object_id_type, obj.speed_limit.from, obj.speed_limit.from = val)
  (osmium::object_id_type, osmium::object_id_type, obj.speed_limit.to, obj.speed_limit.to = val)
  (std::string, std::string const &, obj.speed_limit.tag, obj.speed_limit.tag = val)
  (int, int, obj.speed_limit.value, obj.speed_limit.value = val)
  (std::string, std::string const &, obj.speed_limit.condition, obj.speed_limit.condition = val)
  (std::int32_t, std::int32_t, obj.location.lon(), obj.location.set_lon(val))
  (std::int32_t, std::int32_t, obj.location.lat(), obj.location.set_lat(val)))
// clang-format on

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

int SpeedLimitsDumpCommand(const char *executable, const std::vector<std::string> &arguments)
{
    std::string osm_filename, csv_filename;
    ParseDumpCommandArguments(executable, "speed-dump", arguments, osm_filename, csv_filename);

    // Read OSM input file
    const osmium::io::File input_file(osm_filename);

    // Read ways
    ConditionalSpeedLimitsCollector speed_limits_collector;
    osmium::io::Reader reader1(input_file, osmium::io::read_meta::no, osmium::osm_entity_bits::way);
    osmium::apply(reader1, speed_limits_collector);
    reader1.close();

    // Handle nodes and ways in relations
    osmium::io::Reader reader2(
        input_file, osmium::io::read_meta::no, osmium::osm_entity_bits::node);
    osmium::apply(reader2, speed_limits_collector);
    reader2.close();

    // Prepare output stream
    std::streambuf *buf = std::cout.rdbuf();
    std::ofstream of;
    if (!csv_filename.empty())
    {
        of.open(csv_filename, std::ios::binary);
        buf = of.rdbuf();
    }
    std::ostream stream(buf);

    // Process collected restrictions and print CSV output
    speed_limits_collector.process([&stream](const osmium::Location &location,
                                             osmium::object_id_type from,
                                             osmium::object_id_type to,
                                             const std::string &tag,
                                             int value,
                                             const std::string &condition) {
        stream << from << "," << to << "," << tag << "," << value << ",\"" << condition << "\""
               << "," << std::setprecision(6) << location.lon() << "," << std::setprecision(6)
               << location.lat() << "\n";
    });

    return EXIT_SUCCESS;
}

// Time zone shape polygons loaded in R-tree
// local_time_t is a pair of a time zone shape polygon and the corresponding local time
// rtree_t is a lookup R-tree that maps a geographic point to an index in a local_time_t vector
using point_t = boost::geometry::model::
    point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>>;
using polygon_t = boost::geometry::model::polygon<point_t>;
using box_t = boost::geometry::model::box<point_t>;
using rtree_t =
    boost::geometry::index::rtree<std::pair<box_t, size_t>, boost::geometry::index::rstar<8>>;
using local_time_t = std::pair<polygon_t, struct tm>;

// Function loads time zone shape polygons, computes a zone local time for utc_time,
// creates a lookup R-tree and returns a lambda function that maps a point
// to the corresponding local time
auto LoadLocalTimesRTree(const std::string &tz_shapes_filename, std::time_t utc_time)
{
    // Load time zones shapes and collect local times of utc_time
    auto shphandle = SHPOpen(tz_shapes_filename.c_str(), "rb");
    auto dbfhandle = DBFOpen(tz_shapes_filename.c_str(), "rb");

    BOOST_SCOPE_EXIT(&shphandle, &dbfhandle)
    {
        DBFClose(dbfhandle);
        SHPClose(shphandle);
    }
    BOOST_SCOPE_EXIT_END

    if (!shphandle || !dbfhandle)
    {
        throw osrm::util::exception("failed to open " + tz_shapes_filename + ".shp or " +
                                    tz_shapes_filename + ".dbf file");
    }

    int num_entities, shape_type;
    SHPGetInfo(shphandle, &num_entities, &shape_type, NULL, NULL);
    if (num_entities != DBFGetRecordCount(dbfhandle))
    {
        throw osrm::util::exception("inconsistent " + tz_shapes_filename + ".shp and " +
                                    tz_shapes_filename + ".dbf files");
    }

    const auto tzid = DBFGetFieldIndex(dbfhandle, "TZID");
    if (tzid == -1)
    {
        throw osrm::util::exception("did not find field called 'TZID' in the " +
                                    tz_shapes_filename + ".dbf file");
    }

    // Lambda function that returns local time in the tzname time zone
    // Thread safety: MT-Unsafe const:env
    std::unordered_map<std::string, struct tm> local_time_memo;
    auto get_local_time_in_tz = [utc_time, &local_time_memo](const char *tzname) {
        auto it = local_time_memo.find(tzname);
        if (it == local_time_memo.end())
        {
            struct tm timeinfo;
            setenv("TZ", tzname, 1);
            tzset();
            localtime_r(&utc_time, &timeinfo);
            it = local_time_memo.insert({tzname, timeinfo}).first;
        }

        return it->second;
    };

    // Get all time zone shapes and save local times in a vector
    std::vector<rtree_t::value_type> polygons;
    std::vector<local_time_t> local_times;
    for (int shape = 0; shape < num_entities; ++shape)
    {
        auto object = SHPReadObject(shphandle, shape);
        BOOST_SCOPE_EXIT(&object) { SHPDestroyObject(object); }
        BOOST_SCOPE_EXIT_END

        if (object && object->nSHPType == SHPT_POLYGON)
        {
            // Find time zone polygon and place its bbox in into R-Tree
            polygon_t polygon;
            for (int vertex = 0; vertex < object->nVertices; ++vertex)
            {
                polygon.outer().emplace_back(object->padfX[vertex], object->padfY[vertex]);
            }

            polygons.emplace_back(boost::geometry::return_envelope<box_t>(polygon),
                                  local_times.size());

            // Get time zone name and emplace polygon and local time for the UTC input
            const auto tzname = DBFReadStringAttribute(dbfhandle, shape, tzid);
            local_times.emplace_back(local_time_t{polygon, get_local_time_in_tz(tzname)});

            // std::cout << boost::geometry::dsv(boost::geometry::return_envelope<box_t>(polygon))
            //           << " " << tzname << " " << asctime(&local_times.back().second);
        }
    }

    // Create R-tree for collected shape polygons
    rtree_t rtree(polygons);

    // Return a lambda function that maps the input point and UTC time to the local time
    // binds rtree and local_times
    return [rtree, local_times](const point_t &point) {
        std::vector<rtree_t::value_type> result;
        rtree.query(boost::geometry::index::intersects(point), std::back_inserter(result));
        for (const auto v : result)
        {
            const auto index = v.second;
            if (boost::geometry::within(point, local_times[index].first))
                return local_times[index].second;
        }
        return tm{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    };
}

int RestrictionsCheckCommand(const char *executable, const std::vector<std::string> &arguments)
{
    std::string input_filename, output_filename;
    std::string tz_filename = "tz_world";
    std::time_t utc_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::int64_t restriction_value = 32000;
    ParseCheckCommandArguments(executable,
                               "cond-check",
                               arguments,
                               input_filename,
                               output_filename,
                               tz_filename,
                               utc_time,
                               restriction_value);

    // Prepare input stream
    std::streambuf *input_buffer = std::cin.rdbuf();
    std::ifstream input_file;
    if (!input_filename.empty())
    {
        input_file.open(input_filename, std::ios::binary);
        input_buffer = input_file.rdbuf();
    }
    std::istream input_stream(input_buffer);
    input_stream.unsetf(std::ios::skipws);

    boost::spirit::istream_iterator sfirst(input_stream);
    boost::spirit::istream_iterator slast;

    boost::spirit::line_pos_iterator<boost::spirit::istream_iterator> first(sfirst);
    boost::spirit::line_pos_iterator<boost::spirit::istream_iterator> last(slast);

    // Parse CSV file
    namespace qi = boost::spirit::qi;

    std::vector<LocatedConditionalRestriction> conditional_restrictions;
    qi::rule<decltype(first), LocatedConditionalRestriction()> csv_line =
        qi::ulong_long >> ',' >> qi::ulong_long >> ',' >> qi::ulong_long >> ',' >>
        qi::as_string[+(~qi::lit(','))] >> ',' >> qi::as_string[+(~qi::lit(','))] >> ',' >> '"' >>
        qi::as_string[qi::no_skip[*(~qi::lit('"'))]] >> '"' >> ',' >> qi::double_ >> ',' >>
        qi::double_;
    const auto ok = qi::phrase_parse(first, last, *(csv_line), qi::space, conditional_restrictions);

    if (!ok || first != last)
    {
        osrm::util::Log(logERROR) << input_filename << ":" << first.position() << ": parsing error";
        return EXIT_FAILURE;
    }

    // Load R-tree with local times
    auto get_local_time = LoadLocalTimesRTree(tz_filename, utc_time);

    // Prepare output stream
    std::streambuf *output_buffer = std::cout.rdbuf();
    std::ofstream output_file;
    if (!output_filename.empty())
    {
        output_file.open(output_filename, std::ios::binary);
        output_buffer = output_file.rdbuf();
    }
    std::ostream output_stream(output_buffer);

    // For each conditional restriction if condition is active than print a line
    for (auto &value : conditional_restrictions)
    {
        const auto &location = value.location;
        const auto &restriction = value.restriction;

        // Get local time of the restriction
        const auto &local_time = get_local_time(point_t{location.lon(), location.lat()});

        // TODO: check restriction type [:<transportation mode>][:<direction>]
        // http://wiki.openstreetmap.org/wiki/Conditional_restrictions#Tagging

        // TODO: parsing will fail for combined conditions, e.g. Sa-Su AND weight>7
        // http://wiki.openstreetmap.org/wiki/Conditional_restrictions#Combined_conditions:_AND

        const auto &opening_hours = osrm::util::ParseOpeningHours(restriction.condition);

        if (opening_hours.empty())
        {
            osrm::util::Log(logWARNING)
                << "Condition parsing failed for \"" << restriction.condition << "\" at the turn "
                << restriction.from << " -> " << restriction.via << " -> " << restriction.to;
            continue;
        }

        if (osrm::util::CheckOpeningHours(opening_hours, local_time))
        {
            output_stream << restriction.from << "," << restriction.via << "," << restriction.to
                          << "," << restriction_value << "\n";
        }
    }
    return EXIT_SUCCESS;
}

int SpeedLimitsCheckCommand(const char *executable, const std::vector<std::string> &arguments)
{
    std::string input_filename, output_filename;
    std::string tz_filename = "tz_world";
    std::time_t utc_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::int64_t speed_limit_value = 0;
    ParseCheckCommandArguments(executable,
                               "cond-check",
                               arguments,
                               input_filename,
                               output_filename,
                               tz_filename,
                               utc_time,
                               speed_limit_value);

    // Prepare input stream
    std::streambuf *input_buffer = std::cin.rdbuf();
    std::ifstream input_file;
    if (!input_filename.empty())
    {
        input_file.open(input_filename, std::ios::binary);
        input_buffer = input_file.rdbuf();
    }
    std::istream input_stream(input_buffer);
    input_stream.unsetf(std::ios::skipws);

    boost::spirit::istream_iterator sfirst(input_stream);
    boost::spirit::istream_iterator slast;

    boost::spirit::line_pos_iterator<boost::spirit::istream_iterator> first(sfirst);
    boost::spirit::line_pos_iterator<boost::spirit::istream_iterator> last(slast);

    // Parse CSV file
    namespace qi = boost::spirit::qi;

    std::vector<LocatedConditionalSpeedLimit> speed_limits;
    qi::rule<decltype(first), LocatedConditionalSpeedLimit()> csv_line =
        qi::ulong_long >> ',' >> qi::ulong_long >> ',' >> qi::as_string[+(~qi::lit(','))] >> ',' >>
        qi::int_ >> ',' >> '"' >> qi::as_string[qi::no_skip[*(~qi::lit('"'))]] >> '"' >> ',' >>
        qi::double_ >> ',' >> qi::double_;
    const auto ok = qi::phrase_parse(first, last, *(csv_line), qi::space, speed_limits);

    if (!ok || first != last)
    {
        osrm::util::Log(logERROR) << input_filename << ":" << first.position() << ": parsing error";
        return EXIT_FAILURE;
    }

    // Load R-tree with local times
    auto get_local_time = LoadLocalTimesRTree(tz_filename, utc_time);

    // Prepare output stream
    std::streambuf *output_buffer = std::cout.rdbuf();
    std::ofstream output_file;
    if (!output_filename.empty())
    {
        output_file.open(output_filename, std::ios::binary);
        output_buffer = output_file.rdbuf();
    }
    std::ostream output_stream(output_buffer);

    // For each conditional restriction if condition is active than print a line
    for (auto &value : speed_limits)
    {
        const auto &location = value.location;
        const auto &speed_limit = value.speed_limit;

        // Get local time of the restriction
        const auto &local_time = get_local_time(point_t{location.lon(), location.lat()});

        // TODO: check speed limit type [:<transportation mode>][:<direction>]
        // http://wiki.openstreetmap.org/wiki/Conditional_restrictions#Tagging

        // TODO: parsing will fail for combined conditions, e.g. Sa-Su AND weight>7
        // http://wiki.openstreetmap.org/wiki/Conditional_restrictions#Combined_conditions:_AND

        const auto &opening_hours = osrm::util::ParseOpeningHours(speed_limit.condition);

        if (opening_hours.empty())
        {
            osrm::util::Log(logWARNING) << "Condition parsing failed for \""
                                        << speed_limit.condition << "\" on the segment "
                                        << speed_limit.from << " -> " << speed_limit.to;
            continue;
        }

        if (osrm::util::CheckOpeningHours(opening_hours, local_time))
        {
            output_stream << speed_limit.from << "," << speed_limit.to << "," << speed_limit.value
                          << "\n";
        }
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) try
{
    osrm::util::LogPolicy::GetInstance().Unmute();

    // Parse program argument
    auto arguments = ParseGlobalArguments(argc, argv);
    BOOST_ASSERT(!arguments.empty());

    std::unordered_map<std::string, int (*)(const char *, const std::vector<std::string> &)>
        commands = {{"cond-dump", &RestrictionsDumpCommand},
                    {"cond-check", &RestrictionsCheckCommand},
                    {"speed-dump", &SpeedLimitsDumpCommand},
                    {"speed-check", &SpeedLimitsCheckCommand}};

    auto command = commands.find(arguments.front());
    if (command == commands.end())
    {
        std::cerr << "Unknown command: " << arguments.front() << "\n\n"
                  << "Available commands:\n";
        for (auto &command : commands)
            std::cerr << "    " << command.first << "\n";
        return EXIT_FAILURE;
    }

    arguments.erase(arguments.begin());
    return command->second(argv[0], arguments);
}
catch (const std::exception &e)
{
    osrm::util::Log(logERROR) << e.what();
    return EXIT_FAILURE;
}
