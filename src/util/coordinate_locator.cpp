#include "util/coordinate_locator.hpp"
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

// Function loads time zone shape polygons, computes a zone local time for utc_time,
// creates a lookup R-tree and returns a lambda function that maps a point
// to the corresponding local time
namespace osrm
{
namespace util
{

/*

CoordinateLocator::CoordinateLocator(const std::string &geojson,
                                     const std::string &property_to_index)
{
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(geojson.c_str());
    if (!ok)
    {
        auto code = ok.Code();
        auto offset = ok.Offset();
        throw osrm::util::exception("Failed to parse geojson with error code " +
                                    std::to_string(code) + " malformed at offset " +
                                    std::to_string(offset));
    }
    ConstructRTree(doc, property_to_index);
}

*/

CoordinateLocator::CoordinateLocator(const boost::filesystem::path &geojson_filename,
                                     const std::string &property_to_index)
{
    if (geojson_filename.empty())
        throw osrm::util::exception(std::string("Missing geojson file: ") +
                                    geojson_filename.string());
    boost::filesystem::ifstream file(geojson_filename);
    if (!file.is_open())
        throw osrm::util::exception(std::string("failed to open ") + geojson_filename.string());

    util::Log() << "Parsing " + geojson_filename.string();
    rapidjson::IStreamWrapper isw(file);
    rapidjson::Document geojson;
    geojson.ParseStream(isw);
    if (geojson.HasParseError())
    {
        auto error_code = geojson.GetParseError();
        auto error_offset = geojson.GetErrorOffset();
        throw osrm::util::exception(std::string("Failed to parse ") + geojson_filename.string() +
                                    " with error " + std::to_string(error_code) +
                                    ". JSON malformed at " + std::to_string(error_offset));
    }
    ConstructRTree(geojson, property_to_index);
}

void CoordinateLocator::ConstructRTree(rapidjson::Document &geojson,
                                       const std::string &property_name_to_index)
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
    BOOST_ASSERT(geojson["features"].IsArray());
    const auto &features_array = geojson["features"].GetArray();
    std::vector<rtree_t::value_type> bounding_boxes;
    for (rapidjson::SizeType i = 0; i < features_array.Size(); i++)
    {
        util::validateFeature(features_array[i]);
        // time zone geojson specific checks
        if (!features_array[i]["properties"].GetObject().HasMember(property_name_to_index.c_str()))
        {
            throw osrm::util::exception("Feature is missing " + property_name_to_index +
                                        " member in properties.");
        }
        else if (!features_array[i]["properties"]
                      .GetObject()[property_name_to_index.c_str()]
                      .IsString())
        {
            throw osrm::util::exception("Feature has non-string " + property_name_to_index +
                                        " value.");
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
            bounding_boxes.emplace_back(boost::geometry::return_envelope<box_t>(polygon),
                                        polygons.size());

            // Get time zone name and emplace polygon and local time for the UTC input
            const auto &propertyvalue = features_array[i]
                                            .GetObject()["properties"]
                                            .GetObject()[property_name_to_index.c_str()]
                                            .GetString();
            polygons.push_back(make_pair(polygon, propertyvalue));
        }
        else
        {
            util::Log(logDEBUG) << "Skipping non-polygon shape in geojson file";
        }
    }
    util::Log() << "Parsed " << polygons.size() << " geojson polygons." << std::endl;
    // Create R-tree for collected shape polygons
    rtree = rtree_t(bounding_boxes);
}

boost::optional<std::string> CoordinateLocator::find(const point_t &point) const
{
    std::vector<rtree_t::value_type> results;
    // Search the rtree, and return a list of all the polygons the point is inside
    rtree.query(boost::geometry::index::intersects(point) &&
                    boost::geometry::index::satisfies([&, this](const rtree_t::value_type &v) {
                        return boost::geometry::within(point, polygons[v.second].first);
                    }),
                std::back_inserter(results));

    if (results.empty())
        return boost::none;

    return polygons[results.front().second].second;
}
}
}
