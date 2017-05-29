#include "util/exception.hpp"
#include "util/log.hpp"
#include "util/timezones.hpp"

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

Timezoner::Timezoner(std::string tz_filename, std::time_t utc_time_now)
{
    util::Log() << "Time zone validation based on UTC time : " << utc_time_now;
    // Thread safety: MT-Unsafe const:env
    default_time = *gmtime(&utc_time_now);
    LoadLocalTimesRTree(tz_filename, utc_time_now);
}

void Timezoner::ValidateFeature(const rapidjson::Value &feature, const std::string &filename)
{
    if (!feature.HasMember("type"))
    {
        throw osrm::util::exception("Failed to parse " + filename +
                                    ". Feature is missing type member.");
    } else if (!feature["type"].IsString())
    {
        throw osrm::util::exception("Failed to parse " + filename +
                                    ". Feature non-string type member.");
    }
    if (!feature.HasMember("properties"))
    {
        throw osrm::util::exception("Failed to parse " + filename +
                                    ". Feature is missing properties member.");
    }
    else if (!feature.GetObject()["properties"].IsObject())
    {
        throw osrm::util::exception("Failed to parse " + filename +
                                    ". Feature has non-object properties member.");
    }
    if (!feature["properties"].GetObject().HasMember("TZID"))
    {
        throw osrm::util::exception("Failed to parse " + filename +
                                    ". Feature is missing TZID member in properties.");
    }
    else if (!feature["properties"].GetObject()["TZID"].IsString())
    {
        throw osrm::util::exception("Failed to parse " + filename +
                                    ". Feature has non-string TZID value.");
    }
    if (!feature.HasMember("geometry"))
    {
        throw osrm::util::exception("Failed to parse " + filename +
                                    ". Feature is missing geometry member.");
    }
    else if (!feature.GetObject()["geometry"].IsObject())
    {
        throw osrm::util::exception("Failed to parse " + filename +
                                    ". Feature non-object geometry member.");
    }

    if (!feature["geometry"].GetObject().HasMember("type"))
        throw osrm::util::exception("Failed to parse " + filename +
                                    ". Feature geometry is missing type member.");
    if (!feature["geometry"].GetObject().HasMember("coordinates"))
        throw osrm::util::exception("Failed to parse " + filename +
                                    ". Feature geometry is missing coordinates member.");
}

void Timezoner::LoadLocalTimesRTree(const std::string &tz_shapes_filename, std::time_t utc_time)
{
    if (tz_shapes_filename.empty())
        throw osrm::util::exception("Missing time zone geojson file");
    std::ifstream file(tz_shapes_filename.data());
    if (!file.is_open())
        throw osrm::util::exception("failed to open " + tz_shapes_filename);

    util::Log() << "Parsing " + tz_shapes_filename;
    rapidjson::IStreamWrapper isw(file);
    rapidjson::Document geojson;
    geojson.ParseStream(isw);
    if (geojson.HasParseError())
    {
        auto error_code = geojson.GetParseError();
        auto error_offset = geojson.GetErrorOffset();
        throw osrm::util::exception("Failed to parse " + tz_shapes_filename + " with error " +
                                    std::to_string(error_code) + ". JSON malformed at " + std::to_string(error_offset));
    }
    if (!geojson.HasMember("FeatureCollection"))
        throw osrm::util::exception("Failed to parse " + tz_shapes_filename +
                                    ". Expecting a geojson feature collection.");
    if (!geojson["FeatureCollection"].GetObject().HasMember("features"))
        throw osrm::util::exception("Failed to parse " + tz_shapes_filename +
                                    ". Missing features list.");

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
    const rapidjson::Value &features_array = geojson["features"].GetArray();
    std::vector<rtree_t::value_type> polygons;
    for (rapidjson::SizeType i = 0; i < features_array.Size(); i++)
    {
        ValidateFeature(features_array[i], tz_shapes_filename);
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
                // polygon.outer().emplace_back(object->padfX[vertex], object->padfY[vertex]);
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
            util::Log() << "Skipping non-polygon shape in timezone file " + tz_shapes_filename;
        }
    }
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
