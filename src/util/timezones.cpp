#include "util/exception.hpp"
#include "util/log.hpp"
#include "util/timezones.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/scope_exit.hpp>

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

#include <fstream>
#include <string>
#include <unordered_map>

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
    // Thread safety: MT-Unsafe const:env
    default_time = *gmtime(&utc_time_now);
    rapidjson::Document doc;
    doc.Parse(geojson);
    LoadLocalTimesRTree(doc, utc_time_now);
}

Timezoner::Timezoner(const boost::filesystem::path &tz_shapes_filename, std::time_t utc_time_now)
{
    util::Log() << "Time zone validation based on UTC time : " << utc_time_now;
    // Thread safety: MT-Unsafe const:env
    default_time = *gmtime(&utc_time_now);

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
        throw osrm::util::exception("Failed to parse " + tz_shapes_filename.string() + " with error " +
                                    std::to_string(error_code) + ". JSON malformed at " + std::to_string(error_offset));
    }
    LoadLocalTimesRTree(geojson, utc_time_now);
}

void Timezoner::ValidateCoordinate(const rapidjson::Value &coordinate)
{
    if (!coordinate.IsArray())
        throw osrm::util::exception("Failed to parse time zone file. Feature geometry has a non-array coordinate.");
    if (coordinate.Capacity() != 2)
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature geometry has a malformed coordinate with more than 2 values.");
    } else {
        for (rapidjson::SizeType i = 0; i < coordinate.Size(); i++)
        {
            if (!coordinate[i].IsNumber() && !coordinate[i].IsDouble())
                throw osrm::util::exception("Failed to parse time zone file. Feature geometry has a non-number coordinate.");
        }
    }
}

void Timezoner::ValidateFeature(const rapidjson::Value &feature)
{
    if (!feature.HasMember("type"))
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature is missing type member.");
    } else if (!feature["type"].IsString())
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature has non-string type member.");
    }
    if (!feature.HasMember("properties"))
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature is missing properties member.");
    }
    else if (!feature.GetObject()["properties"].IsObject())
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature has non-object properties member.");
    }
    if (!feature["properties"].GetObject().HasMember("TZID"))
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature is missing TZID member in properties.");
    }
    else if (!feature["properties"].GetObject()["TZID"].IsString())
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature has non-string TZID value.");
    }
    if (!feature.HasMember("geometry"))
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature is missing geometry member.");
    }
    else if (!feature.GetObject()["geometry"].IsObject())
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature non-object geometry member.");
    }

    if (!feature["geometry"].GetObject().HasMember("type"))
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature geometry is missing type member.");
    } else if (!feature["geometry"].GetObject()["type"].IsString()) {
        throw osrm::util::exception("Failed to parse time zone file. Feature geometry has non-string type member.");
    }
    if (!feature["geometry"].GetObject().HasMember("coordinates"))
    {
        throw osrm::util::exception("Failed to parse time zone file. Feature geometry is missing coordinates member.");
    } else if (!feature["geometry"].GetObject()["coordinates"].IsArray()) {
        throw osrm::util::exception("Failed to parse time zone file. Feature geometry has a non-array coordinates member.");
    }
    const auto coord_array = feature["geometry"].GetObject()["coordinates"].GetArray();
    if (coord_array.Empty())
        throw osrm::util::exception("Failed to parse time zone file. Feature geometry coordinates member is empty.");
    if (!coord_array[0].IsArray())
        throw osrm::util::exception("Failed to parse time zone file. Feature geometry coordinates array has non-array outer ring.");
}

void Timezoner::LoadLocalTimesRTree(rapidjson::Document &geojson, std::time_t utc_time)
{
    if (!geojson.HasMember("type"))
        throw osrm::util::exception("Failed to parse time zone file. Missing type member.");
    if (!geojson["type"].IsString())
        throw osrm::util::exception("Failed to parse time zone file. Missing string-based type member.");
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
            setenv("TZ", tzname, 1);
            tzset();
            localtime_r(&utc_time, &timeinfo);
            it = local_time_memo.insert({tzname, timeinfo}).first;
        }

        return it->second;
    };
    BOOST_ASSERT(geojson["features"].IsArray());
    const auto &features_array = geojson["features"].GetArray();
    std::vector<rtree_t::value_type> polygons;
    for (rapidjson::SizeType i = 0; i < features_array.Size(); i++)
    {
        ValidateFeature(features_array[i]);
        const std::string &feat_type = features_array[i].GetObject()["geometry"].GetObject()["type"].GetString();
        if (feat_type == "polygon")
        {
            polygon_t polygon;
            // per geojson spec, the first array of polygon coords is the exterior ring, we only want to access that
            auto coords_outer_array = features_array[i]
                                          .GetObject()["geometry"]
                                          .GetObject()["coordinates"]
                                          .GetArray()[0]
                                          .GetArray();
            for (rapidjson::SizeType i = 0; i < coords_outer_array.Size(); ++i)
            {
                ValidateCoordinate(coords_outer_array[i]);
                const auto &coords = coords_outer_array[i].GetArray();
                polygon.outer().emplace_back(coords[0].GetDouble(), coords[1].GetDouble());
            }
            polygons.emplace_back(boost::geometry::return_envelope<box_t>(polygon),
                                  local_times.size());

            // Get time zone name and emplace polygon and local time for the UTC input
            const auto tzname =
                features_array[i].GetObject()["properties"].GetObject()["TZID"].GetString();
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

struct tm Timezoner::operator()(const point_t &point) const
{
    std::vector<rtree_t::value_type> result;
    rtree.query(boost::geometry::index::intersects(point), std::back_inserter(result));
    for (const auto v : result)
    {
        const auto index = v.second;
        if (boost::geometry::within(point, local_times[index].first))
            return local_times[index].second;
    }
    return default_time;
}
}
}
