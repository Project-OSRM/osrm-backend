#include "updater/csv_source.hpp"

#include "updater/csv_file_parser.hpp"
#include "util/log.hpp"

#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/fusion/include/adapt_adt.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

// clang-format off
BOOST_FUSION_ADAPT_STRUCT(osrm::updater::Segment,
                         (decltype(osrm::updater::Segment::from), from)
                         (decltype(osrm::updater::Segment::to), to))
BOOST_FUSION_ADAPT_STRUCT(osrm::updater::SpeedSource,
                          (decltype(osrm::updater::SpeedSource::speed), speed)
                          (decltype(osrm::updater::SpeedSource::rate), rate))
BOOST_FUSION_ADAPT_STRUCT(osrm::updater::Turn,
                          (decltype(osrm::updater::Turn::from), from)
                          (decltype(osrm::updater::Turn::via), via)
                          (decltype(osrm::updater::Turn::to), to))
BOOST_FUSION_ADAPT_STRUCT(osrm::updater::PenaltySource,
                          (decltype(osrm::updater::PenaltySource::duration), duration)
                          (decltype(osrm::updater::PenaltySource::weight), weight))
// clang-format on
namespace
{
namespace qi = boost::spirit::qi;
}

namespace osrm::updater::csv
{
SegmentLookupTable readSegmentValues(const std::vector<std::string> &paths)
{
    SegmentLookupTable result;
    
    for (const auto &path : paths)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            util::Log(logWARNING) << "Could not open CSV file: " << path;
            continue;
        }
        
        std::string line;
        std::size_t line_number = 0;

        while (std::getline(file, line))
        {
            line_number++;
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#')
                continue;
            
            std::istringstream iss(line);
            std::string from_str, to_str, speed_str;
            
            if (!std::getline(iss, from_str, ',') || 
                !std::getline(iss, to_str, ',') || 
                !std::getline(iss, speed_str, ','))  // Speed can be the last field or followed by rate
            {
                // Try without trailing comma for speed field
                iss.clear();
                iss.str(line);
                if (!std::getline(iss, from_str, ',') || 
                    !std::getline(iss, to_str, ',') || 
                    !std::getline(iss, speed_str))
                {
                    util::Log(logWARNING) << "Invalid CSV format at line " << line_number 
                                          << " in file " << path;
                    continue;
                }
            }
            
            try 
            {
                Segment segment;
                segment.from = std::stoull(from_str);
                segment.to = std::stoull(to_str);
                
                if (segment.from == segment.to)
                {
                    util::Log(logWARNING) << "Empty segment in CSV with node " << segment.from
                                          << " at line " << line_number;
                    continue;
                }
                
                SpeedSource speed_source;
                speed_source.source = 1; // CSV source
                
                // Check for operation prefix
                if (!speed_str.empty())
                {
                    if (speed_str[0] == '*')
                    {
                        speed_source.operation = SpeedSource::MULTIPLY;
                        speed_source.speed = std::stod(speed_str.substr(1));
                    }
                    else if (speed_str[0] == '/')
                    {
                        speed_source.operation = SpeedSource::DIVIDE;
                        speed_source.speed = std::stod(speed_str.substr(1));
                    }
                    else
                    {
                        speed_source.operation = SpeedSource::ABSOLUTE;
                        speed_source.speed = std::stod(speed_str);
                    }
                }
                
                // Optional rate field
                std::string rate_str;
                if (std::getline(iss, rate_str, ',') && !rate_str.empty())
                {
                    speed_source.rate = std::stod(rate_str);
                }
                
                result.lookup.push_back({segment, speed_source});
            }
            catch (const std::exception &e)
            {
                util::Log(logWARNING) << "Error parsing CSV line " << line_number 
                                      << " in file " << path << ": " << e.what();
            }
        }
        
    }
    
    // Sort the lookup table for binary search
    std::sort(result.lookup.begin(), result.lookup.end(), 
              [](const auto &a, const auto &b) { return a.first < b.first; });
    
    return result;
}

TurnLookupTable readTurnValues(const std::vector<std::string> &paths)
{
    CSVFilesParser<Turn, PenaltySource> parser(1,
                                               qi::ulong_long >> ',' >> qi::ulong_long >> ',' >>
                                                   qi::ulong_long,
                                               qi::double_ >> -(',' >> qi::double_));
    return parser(paths);
}
} // namespace osrm::updater::csv
