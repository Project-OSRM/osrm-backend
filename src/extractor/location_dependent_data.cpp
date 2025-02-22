#include "extractor/location_dependent_data.hpp"

#include "util/exception.hpp"
#include "util/geojson_validation.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

#include <boost/geometry/algorithms/equals.hpp>
#include <boost/iterator/function_output_iterator.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace osrm::extractor
{

LocationDependentData::LocationDependentData(const std::vector<std::filesystem::path> &file_paths)
{
    std::vector<rtree_t::value_type> bounding_boxes;
    for (const auto &path : file_paths)
    {
        loadLocationDependentData(path, bounding_boxes);
    }

    // Create R-tree for bounding boxes of collected polygons
    rtree = rtree_t(bounding_boxes);
    util::Log() << "Parsed " << properties.size() << " location-dependent features with "
                << polygons.size() << " GeoJSON polygons";
}

void LocationDependentData::loadLocationDependentData(
    const std::filesystem::path &file_path, std::vector<rtree_t::value_type> &bounding_boxes)
{
    if (file_path.empty())
        return;

    if (!std::filesystem::exists(file_path) || !std::filesystem::is_regular_file(file_path))
    {
        throw osrm::util::exception(std::string("File with location-dependent data ") +
                                    file_path.string() + " does not exists");
    }

    std::ifstream file(file_path.string());
    if (!file.is_open())
        throw osrm::util::exception("failed to open " + file_path.string());

    rapidjson::IStreamWrapper isw(file);
    rapidjson::Document geojson;
    geojson.ParseStream(isw);
    if (geojson.HasParseError())
    {
        throw osrm::util::exception(std::string("Failed to parse ") + file_path.string() + ":" +
                                    std::to_string(geojson.GetErrorOffset()) + " error: " +
                                    rapidjson::GetParseError_En(geojson.GetParseError()));
    }

    BOOST_ASSERT(geojson.HasMember("type"));
    BOOST_ASSERT(geojson["type"].IsString());
    BOOST_ASSERT(std::strcmp(geojson["type"].GetString(), "FeatureCollection") == 0);
    BOOST_ASSERT(geojson.HasMember("features"));
    BOOST_ASSERT(geojson["features"].IsArray());

    const auto &features_array = geojson["features"].GetArray();

    auto convert_value = [](const auto &property) -> property_t
    {
        if (property.IsString())
            return std::string(property.GetString());
        if (property.IsNumber())
            return property.GetDouble();
        if (property.IsBool())
            return property.GetBool();
        return {};
    };

    auto collect_properties = [this, &convert_value](const auto &object) -> std::size_t
    {
        properties_t object_properties;
        for (const auto &property : object)
        {
            object_properties.insert({property.name.GetString(), convert_value(property.value)});
        }
        const std::size_t index = properties.size();
        properties.emplace_back(object_properties);
        return index;
    };

    auto index_polygon = [this, &bounding_boxes](const auto &rings, auto properties_index)
    {
        // At least an outer ring in polygon https://tools.ietf.org/html/rfc7946#section-3.1.6
        BOOST_ASSERT(rings.Size() > 0);

        auto to_point = [](const auto &json) -> point_t
        {
            util::validateCoordinate(json);
            const auto &coords = json.GetArray();
            return {coords[0].GetDouble(), coords[1].GetDouble()};
        };

        std::vector<segment_t> segments;
        auto append_ring_segments = [&segments, &to_point](const auto &coordinates_array) -> box_t
        {
            using coord_t = boost::geometry::traits::coordinate_type<point_t>::type;
            auto x_min = std::numeric_limits<coord_t>::max();
            auto y_min = std::numeric_limits<coord_t>::max();
            auto x_max = std::numeric_limits<coord_t>::min();
            auto y_max = std::numeric_limits<coord_t>::min();
            if (!coordinates_array.Empty())
            {
                point_t curr = to_point(coordinates_array[0]), next;
                for (rapidjson::SizeType i = 1; i < coordinates_array.Size(); ++i, curr = next)
                {
                    next = to_point(coordinates_array[i]);
                    segments.emplace_back(curr, next);
                    x_min = std::min(x_min, next.x());
                    x_max = std::max(x_max, next.x());
                    y_min = std::min(y_min, next.y());
                    y_max = std::max(y_max, next.y());
                }
            }

            return box_t{{x_min, y_min}, {x_max, y_max}};
        };

        auto envelop = append_ring_segments(rings[0].GetArray());
        bounding_boxes.emplace_back(envelop, polygons.size());
        for (rapidjson::SizeType iring = 1; iring < rings.Size(); ++iring)
        {
            append_ring_segments(rings[iring].GetArray());
        }

        constexpr const std::size_t segments_per_band = 10;
        constexpr const std::size_t max_bands = 100000;
        auto num_bands = segments.size() / segments_per_band;
        if (num_bands < 1)
        {
            num_bands = 1;
        }
        else if (num_bands > max_bands)
        {
            num_bands = max_bands;
        }

        polygon_bands_t bands(num_bands);

        const auto y_min = envelop.min_corner().y();
        const auto y_max = envelop.max_corner().y();
        const auto dy = (y_max - y_min) / num_bands;

        for (const auto &segment : segments)
        {
            using coord_t = boost::geometry::traits::coordinate_type<point_t>::type;
            const std::pair<coord_t, coord_t> mm =
                std::minmax(segment.first.y(), segment.second.y());
            const auto band_min = std::min<coord_t>(num_bands - 1, (mm.first - y_min) / dy);
            const auto band_max = std::min<coord_t>(num_bands, ((mm.second - y_min) / dy) + 1);

            for (auto band = band_min; band < band_max; ++band)
            {
                bands[band].push_back(segment);
            }
        }

        polygons.emplace_back(std::make_pair(bands, properties_index));
    };

    for (rapidjson::SizeType ifeature = 0; ifeature < features_array.Size(); ifeature++)
    {
        util::validateFeature(features_array[ifeature]);
        const auto &feature = features_array[ifeature].GetObject();
        const auto &geometry = feature["geometry"].GetObject();
        BOOST_ASSERT(geometry.HasMember("type"));

        // Case-sensitive check of type https://tools.ietf.org/html/rfc7946#section-1.4
        if (std::strcmp(geometry["type"].GetString(), "Polygon") == 0)
        {
            // Collect feature properties and store in polygons vector
            auto properties_index = collect_properties(feature["properties"].GetObject());
            const auto &coordinates = geometry["coordinates"].GetArray();
            index_polygon(coordinates, properties_index);
        }
        else if (std::strcmp(geometry["type"].GetString(), "MultiPolygon") == 0)
        {
            auto properties_index = collect_properties(feature["properties"].GetObject());
            const auto &polygons = geometry["coordinates"].GetArray();
            for (rapidjson::SizeType ipolygon = 0; ipolygon < polygons.Size(); ++ipolygon)
            {
                index_polygon(polygons[ipolygon].GetArray(), properties_index);
            }
        }
    }
}

LocationDependentData::property_t
LocationDependentData::FindByKey(const std::vector<std::size_t> &property_indexes,
                                 const char *key) const
{
    for (auto index : property_indexes)
    {
        const auto &polygon_properties = properties[index];
        const auto it = polygon_properties.find(key);
        if (it != polygon_properties.end())
        {
            return it->second;
        }
    }
    return property_t{};
}

std::vector<std::size_t> LocationDependentData::GetPropertyIndexes(const point_t &point) const
{
    std::vector<std::size_t> result;
    auto inserter = [this, &result](const rtree_t::value_type &rtree_entry)
    {
        const auto properties_index = polygons[rtree_entry.second].second;
        result.push_back(properties_index);
    };

    // Search the R-tree and collect a Lua table of tags that correspond to the location
    rtree.query(
        boost::geometry::index::intersects(point) &&
            boost::geometry::index::satisfies(
                [this, &point](const rtree_t::value_type &v)
                {
                    // Simple point-in-polygon algorithm adapted from
                    // https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html

                    const auto &envelop = v.first;
                    const auto &bands = polygons[v.second].first;

                    const auto y_min = envelop.min_corner().y();
                    const auto y_max = envelop.max_corner().y();
                    const auto dy = (y_max - y_min) / bands.size();

                    std::size_t band = (point.y() - y_min) / dy;
                    if (band >= bands.size())
                    {
                        band = bands.size() - 1;
                    }

                    bool inside = false;

                    for (const auto &segment : bands[band])
                    {
                        const auto point_x = point.x(), point_y = point.y();
                        const auto from_x = segment.first.x(), from_y = segment.first.y();
                        const auto to_x = segment.second.x(), to_y = segment.second.y();

                        if (to_y == from_y)
                        { // handle horizontal segments: check if on boundary or skip
                            if ((to_y == point_y) &&
                                (from_x == point_x || (to_x > point_x) != (from_x > point_x)))
                                return true;
                            continue;
                        }

                        if ((to_y > point_y) != (from_y > point_y))
                        {
                            const auto ax = to_x - from_x;
                            const auto ay = to_y - from_y;
                            const auto tx = point_x - from_x;
                            const auto ty = point_y - from_y;

                            const auto cross_product = tx * ay - ax * ty;

                            if (cross_product == 0)
                                return true;

                            if ((ay > 0) == (cross_product > 0))
                            {
                                inside = !inside;
                            }
                        }
                    }

                    return inside;
                }),
        boost::make_function_output_iterator(std::ref(inserter)));

    return result;
}
} // namespace osrm::extractor
