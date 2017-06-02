#include "util/timezones.hpp"
#include "util/exception.hpp"
#include "util/geojson_validation.hpp"
#include "util/log.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>
#include <boost/scope_exit.hpp>

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

#include <fstream>
#include <regex>
#include <string>
#include <unordered_map>

#include <time.h>

// Function loads time zone shape polygons, computes a zone local time for utc_time,
// creates a lookup R-tree and returns a lambda function that maps a point
// to the corresponding local time
namespace osrm
{
namespace updater
{

Timezoner::Timezoner(const char geojson[], std::time_t utc_time_now)
{
    util::Log() << "Time zone validation based on UTC time : " << utc_time_now;
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(geojson);
    if (!ok)
    {
        auto code = ok.Code();
        auto offset = ok.Offset();
        throw osrm::util::exception("Failed to parse timezone geojson with error code " +
                                    std::to_string(code) + " malformed at offset " +
                                    std::to_string(offset));
    }
    LoadLocalTimesRTree(doc, utc_time_now);
}

Timezoner::Timezoner(const boost::filesystem::path &tz_shapes_filename, std::time_t utc_time_now)
{
    util::Log() << "Time zone validation based on UTC time : " << utc_time_now;

    if (tz_shapes_filename.empty())
        throw osrm::util::exception("Missing time zone geojson file");
    boost::filesystem::ifstream file(tz_shapes_filename);
    if (!file.is_open())
        throw osrm::util::exception("failed to open " + tz_shapes_filename.string());

    util::Log() << "Parsing " + tz_shapes_filename.string();
    rapidjson::IStreamWrapper isw(file);
    rapidjson::Document geojson;
    geojson.ParseStream(isw);
    if (geojson.HasParseError())
    {
        auto error_code = geojson.GetParseError();
        auto error_offset = geojson.GetErrorOffset();
        throw osrm::util::exception("Failed to parse " + tz_shapes_filename.string() +
                                    " with error " + std::to_string(error_code) +
                                    ". JSON malformed at " + std::to_string(error_offset));
    }
    LoadLocalTimesRTree(geojson, utc_time_now);
}

void Timezoner::LoadLocalTimesRTree(rapidjson::Document &geojson, std::time_t utc_time)
{
    if (!geojson.HasMember("type"))
        throw osrm::util::exception("Failed to parse time zone file. Missing type member.");
    if (!geojson["type"].IsString())
        throw osrm::util::exception(
            "Failed to parse time zone file. Missing string-based type member.");
    if (geojson["type"].GetString() != std::string("FeatureCollection"))
        throw osrm::util::exception(
            "Failed to parse time zone file. Geojson is not of FeatureCollection type");
    if (!geojson.HasMember("features"))
        throw osrm::util::exception("Failed to parse time zone file. Missing features list.");

    // Lambda function that returns local time in the tzname time zone
    // Thread safety: MT-Unsafe const:env
    std::unordered_map<std::string, struct tm> local_time_memo;
    auto get_local_time_in_tz = [utc_time, &local_time_memo](const char *tzname) {
        auto it = local_time_memo.find(tzname);
        if (it == local_time_memo.end())
        {
            struct tm timeinfo;
#if defined(_WIN32)
            _putenv_s("TZ", tzname);
            _tzset();
            localtime_s(&timeinfo, &utc_time);
#else
            setenv("TZ", tzname, 1);
            tzset();
            localtime_r(&utc_time, &timeinfo);
#endif
            it = local_time_memo.insert({tzname, timeinfo}).first;
        }

        return it->second;
    };
    BOOST_ASSERT(geojson["features"].IsArray());
    const auto &features_array = geojson["features"].GetArray();
    std::vector<rtree_t::value_type> polygons;
    for (rapidjson::SizeType i = 0; i < features_array.Size(); i++)
    {
        util::validateFeature(features_array[i]);
        // time zone geojson specific checks
        if (!features_array[i]["properties"].GetObject().HasMember("TZID") &&
            !features_array[i]["properties"].GetObject().HasMember("tzid"))
        {
            throw osrm::util::exception("Feature is missing TZID member in properties.");
        }
        else if (!features_array[i]["properties"].GetObject()["tzid"].IsString())
        {
            throw osrm::util::exception("Feature has non-string TZID value.");
        }
        const std::string &feat_type =
            features_array[i].GetObject()["geometry"].GetObject()["type"].GetString();
        std::regex polygon_match("polygon", std::regex::icase);
        std::smatch res;
        if (std::regex_match(feat_type, res, polygon_match))
        {
            polygon_t polygon;
            // per geojson spec, the first array of polygon coords is the exterior ring, we only
            // want to access that
            auto coords_outer_array = features_array[i]
                                          .GetObject()["geometry"]
                                          .GetObject()["coordinates"]
                                          .GetArray()[0]
                                          .GetArray();
            for (rapidjson::SizeType i = 0; i < coords_outer_array.Size(); ++i)
            {
                util::validateCoordinate(coords_outer_array[i]);
                const auto &coords = coords_outer_array[i].GetArray();
                polygon.outer().emplace_back(coords[0].GetDouble(), coords[1].GetDouble());
            }
            polygons.emplace_back(boost::geometry::return_envelope<box_t>(polygon),
                                  local_times.size());

            // Get time zone name and emplace polygon and local time for the UTC input
            const auto &tzname =
                features_array[i].GetObject()["properties"].GetObject()["tzid"].GetString();
            local_times.push_back(local_time_t{polygon, get_local_time_in_tz(tzname)});
        }
        else
        {
            util::Log(logDEBUG) << "Skipping non-polygon shape in timezone file";
        }
    }
    util::Log() << "Parsed " << polygons.size() << "time zone polygons." << std::endl;
    // Create R-tree for collected shape polygons
    rtree = rtree_t(polygons);
}

boost::optional<struct tm> Timezoner::operator()(const point_t &point) const
{
    std::vector<rtree_t::value_type> result;
    rtree.query(boost::geometry::index::intersects(point), std::back_inserter(result));
    for (const auto v : result)
    {
        const auto index = v.second;
        if (boost::geometry::within(point, local_times[index].first))
            return local_times[index].second;
    }
    return boost::none;
}
}
}
