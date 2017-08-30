#include "extractor/location_dependent_data.hpp"

#include "util/exception.hpp"
#include "util/geojson_validation.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

#include <boost/filesystem.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/geometry/algorithms/equals.hpp>

#include <fstream>
#include <string>

namespace osrm
{
namespace extractor
{

LocationDependentData::LocationDependentData(const boost::filesystem::path &path)
{
    loadLocationDependentData(path);
}

LocationDependentData::LocationDependentData(const std::vector<boost::filesystem::path> &file_paths)
{
    for (const auto &path : file_paths)
    {
        loadLocationDependentData(path);
    }
}

void LocationDependentData::loadLocationDependentData(const boost::filesystem::path &file_path)
{
    if (file_path.empty())
        return;

    if (!boost::filesystem::exists(file_path) || !boost::filesystem::is_regular_file(file_path))
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
    std::vector<rtree_t::value_type> bounding_boxes;

    auto convert_value = [](const auto &property) -> property_t {
        if (property.IsString())
            return std::string(property.GetString());
        if (property.IsNumber())
            return property.GetDouble();
        if (property.IsBool())
            return property.GetBool();
        return {};
    };

    auto collect_properties = [this, &convert_value](const auto &object) -> std::size_t {
        properties_t object_properties;
        for (const auto &property : object)
        {
            object_properties.insert({property.name.GetString(), convert_value(property.value)});
        }
        const std::size_t index = properties.size();
        properties.emplace_back(object_properties);
        return index;
    };

    auto convert_to_ring = [](const auto &coordinates_array) -> polygon_t::ring_type {
        polygon_t::ring_type ring;
        for (rapidjson::SizeType i = 0; i < coordinates_array.Size(); ++i)
        {
            util::validateCoordinate(coordinates_array[i]);
            const auto &coords = coordinates_array[i].GetArray();
            ring.emplace_back(coords[0].GetDouble(), coords[1].GetDouble());
        }
        return ring;
    };

    auto index_polygon = [this, &bounding_boxes, &convert_to_ring](const auto &rings,
                                                                   auto properties_index) {
        // https://tools.ietf.org/html/rfc7946#section-3.1.6
        BOOST_ASSERT(rings.Size() > 0);
        polygon_t polygon;
        polygon.outer() = convert_to_ring(rings[0].GetArray());
        for (rapidjson::SizeType iring = 1; iring < rings.Size(); ++iring)
        {
            polygon.inners().emplace_back(convert_to_ring(rings[iring].GetArray()));
        }
        auto envelop = boost::geometry::return_envelope<box_t>(polygon);
        bounding_boxes.emplace_back(envelop, polygons.size());

        // here is a part of ExtractPolygon::ExtractPolygon code
        // TODO: remove copy overhead
        constexpr const int32_t segments_per_band = 10;
        constexpr const int32_t max_bands = 10000;
        const auto y_min = envelop.min_corner().y();
        const auto y_max = envelop.max_corner().y();

        std::vector<segment_t> segments;
        auto add_ring = [&segments](const auto &ring) {
            auto it = ring.begin();
            const auto end = ring.end();

            BOOST_ASSERT(it != end);
            auto last_it = it++;
            while (it != end)
            {
                segments.emplace_back(*last_it, *it);
                last_it = it++;
            }
        };

        add_ring(polygon.outer());
        for (const auto &ring : polygon.inners())
            add_ring(ring);

        int32_t num_bands = static_cast<int32_t>(segments.size()) / segments_per_band;
        if (num_bands < 1)
        {
            num_bands = 1;
        }
        else if (num_bands > max_bands)
        {
            num_bands = max_bands;
        }

        polygon_bands_t bands(num_bands);
        const auto dy = (y_max - y_min) / num_bands;

        for (const auto &segment : segments)
        {
            using coord_t = boost::geometry::traits::coordinate_type<point_t>::type;
            const std::pair<coord_t, coord_t> mm =
                std::minmax(segment.first.y(), segment.second.y());
            const auto band_min = std::min<coord_t>(num_bands - 1, (mm.first - y_min) / dy);
            const auto band_max = std::min<coord_t>(
                num_bands, ((mm.second - y_min) / dy) + 1); // TODO: use integer coordinates

            for (auto band = band_min; band < band_max; ++band)
            {
                bands[band].push_back(segment);
            }
        }
        // EOC

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

    // Create R-tree for bounding boxes of collected polygons
    rtree = rtree_t(bounding_boxes);
    util::Log() << "Parsed " << properties.size() << " location-dependent features with "
                << polygons.size() << " GeoJSON polygons";
}

LocationDependentData::properties_t LocationDependentData::operator()(const point_t &point) const
{
    properties_t result;

    auto merger = [this, &result](const rtree_t::value_type &rtree_entry) {
        const auto &polygon_properties = properties[polygons[rtree_entry.second].second];
        result.insert(polygon_properties.begin(), polygon_properties.end());
    };

    // Search the R-tree and collect a Lua table of tags that correspond to the location
    rtree.query(boost::geometry::index::intersects(point) &&
                    boost::geometry::index::satisfies([this, &point](const rtree_t::value_type &v) {

                        // Simple point-in-polygon algorithm adapted from
                        // https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html

                        const auto &envelop = v.first;
                        const auto &bands = polygons[v.second].first;

                        const auto y_min = envelop.min_corner().y();
                        const auto y_max = envelop.max_corner().y();
                        auto dy = (y_max - y_min) / bands.size();

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
                boost::make_function_output_iterator(std::ref(merger)));

    return result;
}

LocationDependentData::properties_t LocationDependentData::operator()(const osmium::Way &way) const
{
    // HEURISTIC: use a single node (last) of the way to localize the way
    // For more complicated scenarios a proper merging of multiple tags
    // at one or many locations must be provided
    const auto &nodes = way.nodes();
    const auto &location = nodes.back().location();
    const point_t point(location.lon(), location.lat());

    return operator()(point);
}
}
}
