#include "extractor/location_dependent_data.hpp"

#include "util/exception.hpp"
#include "util/geojson_validation.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

#include <boost/filesystem.hpp>
#include <boost/function_output_iterator.hpp>

#include <fstream>
#include <string>

namespace osrm
{
namespace extractor
{

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
        polygons.emplace_back(std::make_pair(polygon, properties_index));
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

namespace
{
struct table_setter : public boost::static_visitor<>
{
    table_setter(sol::table &table, const std::string &key) : table(table), key(key) {}
    template <typename T> void operator()(const T &value) const { table.set(key, value); }
    void operator()(const boost::blank &) const { /* ignore */}

    sol::table &table;
    const std::string &key;
};
}

sol::table LocationDependentData::operator()(sol::state &state, const osmium::Way &way) const
{
    if (rtree.empty())
        return sol::make_object(state, sol::nil);

    // HEURISTIC: use a single node (last) of the way to localize the way
    // For more complicated scenarios a proper merging of multiple tags
    // at one or many locations must be provided
    const auto &nodes = way.nodes();
    const auto &location = nodes.back().location();
    const point_t point(location.lon(), location.lat());

    auto table = sol::table(state, sol::create);
    auto merger = [this, &table](const rtree_t::value_type &rtree_entry) {
        for (const auto &key_value : properties[polygons[rtree_entry.second].second])
        {
            boost::apply_visitor(table_setter(table, key_value.first), key_value.second);
        }
    };

    // Search the R-tree and collect a Lua table of tags that correspond to the location
    rtree.query(boost::geometry::index::intersects(point) &&
                    boost::geometry::index::satisfies([this, &point](const rtree_t::value_type &v) {
                        return boost::geometry::within(point, polygons[v.second].first);
                    }),
                boost::make_function_output_iterator(std::ref(merger)));

    return table;
}
}
}
