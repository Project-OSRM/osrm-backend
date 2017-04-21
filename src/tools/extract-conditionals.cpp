#include "conditionals/extract-conditionals.hpp"

#include "util/exception.hpp"
#include "util/opening_hours.hpp"
#include "util/timezones.hpp"
#include "util/version.hpp"

#include <cstdlib>
#include <iostream>
#include <unordered_map>
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
