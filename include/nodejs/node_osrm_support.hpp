#ifndef OSRM_BINDINGS_NODE_SUPPORT_HPP
#define OSRM_BINDINGS_NODE_SUPPORT_HPP

#include "nodejs/json_v8_renderer.hpp"
#include "util/json_renderer.hpp"

#include "osrm/approach.hpp"
#include "osrm/bearing.hpp"
#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/osrm.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/status.hpp"
#include "osrm/storage_config.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/tile_parameters.hpp"
#include "osrm/trip_parameters.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <exception>
#include <memory>
#include <utility>

namespace node_osrm
{

using engine_config_ptr = std::unique_ptr<osrm::EngineConfig>;
using route_parameters_ptr = std::unique_ptr<osrm::RouteParameters>;
using trip_parameters_ptr = std::unique_ptr<osrm::TripParameters>;
using tile_parameters_ptr = std::unique_ptr<osrm::TileParameters>;
using match_parameters_ptr = std::unique_ptr<osrm::MatchParameters>;
using nearest_parameters_ptr = std::unique_ptr<osrm::NearestParameters>;
using table_parameters_ptr = std::unique_ptr<osrm::TableParameters>;

struct PluginParameters
{
    bool renderJSONToBuffer = false;
};

using ObjectOrString = typename mapbox::util::variant<osrm::json::Object, std::string>;

template <typename ResultT> inline v8::Local<v8::Value> render(const ResultT &result);

template <> v8::Local<v8::Value> inline render(const std::string &result)
{
    return Nan::CopyBuffer(result.data(), result.size()).ToLocalChecked();
}

template <> v8::Local<v8::Value> inline render(const ObjectOrString &result)
{
    if (result.is<osrm::json::Object>())
    {
        // Convert osrm::json object tree into matching v8 object tree
        v8::Local<v8::Value> value;
        renderToV8(value, result.get<osrm::json::Object>());
        return value;
    }
    else
    {
        // Return the string object as a node Buffer
        return Nan::CopyBuffer(result.get<std::string>().data(), result.get<std::string>().size())
            .ToLocalChecked();
    }
}

inline void ParseResult(const osrm::Status &result_status, osrm::json::Object &result)
{
    const auto code_iter = result.values.find("code");
    const auto end_iter = result.values.end();

    BOOST_ASSERT(code_iter != end_iter);

    if (result_status == osrm::Status::Error)
    {
        throw std::logic_error(code_iter->second.get<osrm::json::String>().value.c_str());
    }

    result.values.erase(code_iter);
    const auto message_iter = result.values.find("message");
    if (message_iter != end_iter)
    {
        result.values.erase(message_iter);
    }
}

inline void ParseResult(const osrm::Status & /*result_status*/, const std::string & /*unused*/) {}

inline engine_config_ptr argumentsToEngineConfig(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    Nan::HandleScope scope;
    auto engine_config = std::make_unique<osrm::EngineConfig>();

    if (args.Length() == 0)
    {
        return engine_config;
    }
    else if (args.Length() > 1)
    {
        Nan::ThrowError("Only accepts one parameter");
        return engine_config_ptr();
    }

    BOOST_ASSERT(args.Length() == 1);

    if (args[0]->IsString())
    {
        engine_config->storage_config = osrm::StorageConfig(
            *v8::String::Utf8Value(Nan::To<v8::String>(args[0]).ToLocalChecked()));
        engine_config->use_shared_memory = false;
        return engine_config;
    }
    else if (!args[0]->IsObject())
    {
        Nan::ThrowError("Parameter must be a path or options object");
        return engine_config_ptr();
    }

    BOOST_ASSERT(args[0]->IsObject());
    auto params = Nan::To<v8::Object>(args[0]).ToLocalChecked();

    auto path = params->Get(Nan::New("path").ToLocalChecked());
    if (path.IsEmpty())
        return engine_config_ptr();

    auto memory_file = params->Get(Nan::New("memory_file").ToLocalChecked());
    if (memory_file.IsEmpty())
        return engine_config_ptr();

    auto shared_memory = params->Get(Nan::New("shared_memory").ToLocalChecked());
    if (shared_memory.IsEmpty())
        return engine_config_ptr();

    auto mmap_memory = params->Get(Nan::New("mmap_memory").ToLocalChecked());
    if (mmap_memory.IsEmpty())
        return engine_config_ptr();

    if (!memory_file->IsUndefined())
    {
        if (path->IsUndefined())
        {
            Nan::ThrowError("memory_file option requires a path to a file.");
            return engine_config_ptr();
        }

        engine_config->memory_file =
            *v8::String::Utf8Value(Nan::To<v8::String>(memory_file).ToLocalChecked());
    }

    auto dataset_name = params->Get(Nan::New("dataset_name").ToLocalChecked());
    if (dataset_name.IsEmpty())
        return engine_config_ptr();
    if (!dataset_name->IsUndefined())
    {
        if (dataset_name->IsString())
        {
            engine_config->dataset_name =
                *v8::String::Utf8Value(Nan::To<v8::String>(dataset_name).ToLocalChecked());
        }
        else
        {
            Nan::ThrowError("dataset_name needs to be a string");
            return engine_config_ptr();
        }
    }

    if (!path->IsUndefined())
    {
        engine_config->storage_config =
            osrm::StorageConfig(*v8::String::Utf8Value(Nan::To<v8::String>(path).ToLocalChecked()));

        engine_config->use_shared_memory = false;
    }
    if (!shared_memory->IsUndefined())
    {
        if (shared_memory->IsBoolean())
        {
            engine_config->use_shared_memory = Nan::To<bool>(shared_memory).FromJust();
        }
        else
        {
            Nan::ThrowError("Shared_memory option must be a boolean");
            return engine_config_ptr();
        }
    }
    if (!mmap_memory->IsUndefined())
    {
        if (mmap_memory->IsBoolean())
        {
            engine_config->use_mmap = Nan::To<bool>(mmap_memory).FromJust();
        }
        else
        {
            Nan::ThrowError("mmap_memory option must be a boolean");
            return engine_config_ptr();
        }
    }

    if (path->IsUndefined() && !engine_config->use_shared_memory)
    {
        Nan::ThrowError("Shared_memory must be enabled if no path is "
                        "specified");
        return engine_config_ptr();
    }

    auto algorithm = params->Get(Nan::New("algorithm").ToLocalChecked());
    if (algorithm.IsEmpty())
        return engine_config_ptr();

    if (algorithm->IsString())
    {
        auto algorithm_str = Nan::To<v8::String>(algorithm).ToLocalChecked();
        if (*v8::String::Utf8Value(algorithm_str) == std::string("CH"))
        {
            engine_config->algorithm = osrm::EngineConfig::Algorithm::CH;
        }
        else if (*v8::String::Utf8Value(algorithm_str) == std::string("CoreCH"))
        {
            engine_config->algorithm = osrm::EngineConfig::Algorithm::CH;
        }
        else if (*v8::String::Utf8Value(algorithm_str) == std::string("MLD"))
        {
            engine_config->algorithm = osrm::EngineConfig::Algorithm::MLD;
        }
        else
        {
            Nan::ThrowError("algorithm option must be one of 'CH', 'CoreCH', or 'MLD'.");
            return engine_config_ptr();
        }
    }
    else if (!algorithm->IsUndefined())
    {
        Nan::ThrowError("algorithm option must be a string and one of 'CH', 'CoreCH', or 'MLD'.");
        return engine_config_ptr();
    }

    // Set EngineConfig system-wide limits on construction, if requested

    auto max_locations_trip = params->Get(Nan::New("max_locations_trip").ToLocalChecked());
    auto max_locations_viaroute = params->Get(Nan::New("max_locations_viaroute").ToLocalChecked());
    auto max_locations_distance_table =
        params->Get(Nan::New("max_locations_distance_table").ToLocalChecked());
    auto max_locations_map_matching =
        params->Get(Nan::New("max_locations_map_matching").ToLocalChecked());
    auto max_results_nearest = params->Get(Nan::New("max_results_nearest").ToLocalChecked());
    auto max_alternatives = params->Get(Nan::New("max_alternatives").ToLocalChecked());
    auto max_radius_map_matching =
        params->Get(Nan::New("max_radius_map_matching").ToLocalChecked());

    if (!max_locations_trip->IsUndefined() && !max_locations_trip->IsNumber())
    {
        Nan::ThrowError("max_locations_trip must be an integral number");
        return engine_config_ptr();
    }
    if (!max_locations_viaroute->IsUndefined() && !max_locations_viaroute->IsNumber())
    {
        Nan::ThrowError("max_locations_viaroute must be an integral number");
        return engine_config_ptr();
    }
    if (!max_locations_distance_table->IsUndefined() && !max_locations_distance_table->IsNumber())
    {
        Nan::ThrowError("max_locations_distance_table must be an integral number");
        return engine_config_ptr();
    }
    if (!max_locations_map_matching->IsUndefined() && !max_locations_map_matching->IsNumber())
    {
        Nan::ThrowError("max_locations_map_matching must be an integral number");
        return engine_config_ptr();
    }
    if (!max_results_nearest->IsUndefined() && !max_results_nearest->IsNumber())
    {
        Nan::ThrowError("max_results_nearest must be an integral number");
        return engine_config_ptr();
    }
    if (!max_alternatives->IsUndefined() && !max_alternatives->IsNumber())
    {
        Nan::ThrowError("max_alternatives must be an integral number");
        return engine_config_ptr();
    }

    if (max_locations_trip->IsNumber())
        engine_config->max_locations_trip = static_cast<int>(max_locations_trip->NumberValue());
    if (max_locations_viaroute->IsNumber())
        engine_config->max_locations_viaroute =
            static_cast<int>(max_locations_viaroute->NumberValue());
    if (max_locations_distance_table->IsNumber())
        engine_config->max_locations_distance_table =
            static_cast<int>(max_locations_distance_table->NumberValue());
    if (max_locations_map_matching->IsNumber())
        engine_config->max_locations_map_matching =
            static_cast<int>(max_locations_map_matching->NumberValue());
    if (max_results_nearest->IsNumber())
        engine_config->max_results_nearest = static_cast<int>(max_results_nearest->NumberValue());
    if (max_alternatives->IsNumber())
        engine_config->max_alternatives = static_cast<int>(max_alternatives->NumberValue());
    if (max_radius_map_matching->IsNumber())
        engine_config->max_radius_map_matching =
            static_cast<double>(max_radius_map_matching->NumberValue());

    return engine_config;
}

inline boost::optional<std::vector<osrm::Coordinate>>
parseCoordinateArray(const v8::Local<v8::Array> &coordinates_array)
{
    Nan::HandleScope scope;
    boost::optional<std::vector<osrm::Coordinate>> resulting_coordinates;
    std::vector<osrm::Coordinate> temp_coordinates;

    for (uint32_t i = 0; i < coordinates_array->Length(); ++i)
    {
        v8::Local<v8::Value> coordinate = coordinates_array->Get(i);
        if (coordinate.IsEmpty())
            return resulting_coordinates;

        if (!coordinate->IsArray())
        {
            Nan::ThrowError("Coordinates must be an array of (lon/lat) pairs");
            return resulting_coordinates;
        }

        v8::Local<v8::Array> coordinate_pair = v8::Local<v8::Array>::Cast(coordinate);
        if (coordinate_pair->Length() != 2)
        {
            Nan::ThrowError("Coordinates must be an array of (lon/lat) pairs");
            return resulting_coordinates;
        }

        if (!coordinate_pair->Get(0)->IsNumber() || !coordinate_pair->Get(1)->IsNumber())
        {
            Nan::ThrowError("Each member of a coordinate pair must be a number");
            return resulting_coordinates;
        }

        double lon = coordinate_pair->Get(0)->NumberValue();
        double lat = coordinate_pair->Get(1)->NumberValue();

        if (std::isnan(lon) || std::isnan(lat) || std::isinf(lon) || std::isinf(lat))
        {
            Nan::ThrowError("Lng/Lat coordinates must be valid numbers");
            return resulting_coordinates;
        }

        if (lon > 180 || lon < -180 || lat > 90 || lat < -90)
        {
            Nan::ThrowError("Lng/Lat coordinates must be within world bounds "
                            "(-180 < lng < 180, -90 < lat < 90)");
            return resulting_coordinates;
        }

        temp_coordinates.emplace_back(osrm::util::FloatLongitude{std::move(lon)},
                                      osrm::util::FloatLatitude{std::move(lat)});
    }

    resulting_coordinates = boost::make_optional(std::move(temp_coordinates));
    return resulting_coordinates;
}

// Parses all the non-service specific parameters
template <typename ParamType>
inline bool argumentsToParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                                 ParamType &params,
                                 bool requires_multiple_coordinates)
{
    Nan::HandleScope scope;

    if (args.Length() < 2)
    {
        Nan::ThrowTypeError("Two arguments required");
        return false;
    }

    if (!args[0]->IsObject())
    {
        Nan::ThrowTypeError("First arg must be an object");
        return false;
    }

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();

    v8::Local<v8::Value> coordinates = obj->Get(Nan::New("coordinates").ToLocalChecked());
    if (coordinates.IsEmpty())
        return false;

    if (coordinates->IsUndefined())
    {
        Nan::ThrowError("Must provide a coordinates property");
        return false;
    }
    else if (coordinates->IsArray())
    {
        auto coordinates_array = v8::Local<v8::Array>::Cast(coordinates);
        if (coordinates_array->Length() < 2 && requires_multiple_coordinates)
        {
            Nan::ThrowError("At least two coordinates must be provided");
            return false;
        }
        else if (!requires_multiple_coordinates && coordinates_array->Length() != 1)
        {
            Nan::ThrowError("Exactly one coordinate pair must be provided");
            return false;
        }
        auto maybe_coordinates = parseCoordinateArray(coordinates_array);
        if (maybe_coordinates)
        {
            std::copy(maybe_coordinates->begin(),
                      maybe_coordinates->end(),
                      std::back_inserter(params->coordinates));
        }
        else
        {
            return false;
        }
    }
    else if (!coordinates->IsUndefined())
    {
        BOOST_ASSERT(!coordinates->IsArray());
        Nan::ThrowError("Coordinates must be an array of (lon/lat) pairs");
        return false;
    }

    if (obj->Has(Nan::New("approaches").ToLocalChecked()))
    {
        v8::Local<v8::Value> approaches = obj->Get(Nan::New("approaches").ToLocalChecked());
        if (approaches.IsEmpty())
            return false;

        if (!approaches->IsArray())
        {
            Nan::ThrowError("Approaches must be an arrays of strings");
            return false;
        }

        auto approaches_array = v8::Local<v8::Array>::Cast(approaches);

        if (approaches_array->Length() != params->coordinates.size())
        {
            Nan::ThrowError("Approaches array must have the same length as coordinates array");
            return false;
        }

        for (uint32_t i = 0; i < approaches_array->Length(); ++i)
        {
            v8::Local<v8::Value> approach_raw = approaches_array->Get(i);
            if (approach_raw.IsEmpty())
                return false;

            if (approach_raw->IsNull())
            {
                params->approaches.emplace_back();
            }
            else if (approach_raw->IsString())
            {
                const Nan::Utf8String approach_utf8str(approach_raw);
                std::string approach_str{*approach_utf8str,
                                         *approach_utf8str + approach_utf8str.length()};
                if (approach_str == "curb")
                {
                    params->approaches.push_back(osrm::Approach::CURB);
                }
                else if (approach_str == "unrestricted")
                {
                    params->approaches.push_back(osrm::Approach::UNRESTRICTED);
                }
                else
                {
                    Nan::ThrowError("'approaches' param must be one of [curb, unrestricted]");
                    return false;
                }
            }
            else
            {
                Nan::ThrowError("Approach must be a string: [curb, unrestricted] or null");
                return false;
            }
        }
    }

    if (obj->Has(Nan::New("bearings").ToLocalChecked()))
    {
        v8::Local<v8::Value> bearings = obj->Get(Nan::New("bearings").ToLocalChecked());
        if (bearings.IsEmpty())
            return false;

        if (!bearings->IsArray())
        {
            Nan::ThrowError("Bearings must be an array of arrays of numbers");
            return false;
        }

        auto bearings_array = v8::Local<v8::Array>::Cast(bearings);

        if (bearings_array->Length() != params->coordinates.size())
        {
            Nan::ThrowError("Bearings array must have the same length as coordinates array");
            return false;
        }

        for (uint32_t i = 0; i < bearings_array->Length(); ++i)
        {
            v8::Local<v8::Value> bearing_raw = bearings_array->Get(i);
            if (bearing_raw.IsEmpty())
                return false;

            if (bearing_raw->IsNull())
            {
                params->bearings.emplace_back();
            }
            else if (bearing_raw->IsArray())
            {
                auto bearing_pair = v8::Local<v8::Array>::Cast(bearing_raw);
                if (bearing_pair->Length() == 2)
                {
                    if (!bearing_pair->Get(0)->IsNumber() || !bearing_pair->Get(1)->IsNumber())
                    {
                        Nan::ThrowError("Bearing values need to be numbers in range 0..360");
                        return false;
                    }

                    const auto bearing = static_cast<short>(bearing_pair->Get(0)->NumberValue());
                    const auto range = static_cast<short>(bearing_pair->Get(1)->NumberValue());

                    if (bearing < 0 || bearing > 360 || range < 0 || range > 180)
                    {
                        Nan::ThrowError("Bearing values need to be in range 0..360, 0..180");
                        return false;
                    }

                    params->bearings.push_back(osrm::Bearing{bearing, range});
                }
                else
                {
                    Nan::ThrowError("Bearing must be an array of [bearing, range] or null");
                    return false;
                }
            }
            else
            {
                Nan::ThrowError("Bearing must be an array of [bearing, range] or null");
                return false;
            }
        }
    }

    if (obj->Has(Nan::New("hints").ToLocalChecked()))
    {
        v8::Local<v8::Value> hints = obj->Get(Nan::New("hints").ToLocalChecked());
        if (hints.IsEmpty())
            return false;

        if (!hints->IsArray())
        {
            Nan::ThrowError("Hints must be an array of strings/null");
            return false;
        }

        v8::Local<v8::Array> hints_array = v8::Local<v8::Array>::Cast(hints);

        if (hints_array->Length() != params->coordinates.size())
        {
            Nan::ThrowError("Hints array must have the same length as coordinates array");
            return false;
        }

        for (uint32_t i = 0; i < hints_array->Length(); ++i)
        {
            v8::Local<v8::Value> hint = hints_array->Get(i);
            if (hint.IsEmpty())
                return false;

            if (hint->IsString())
            {
                if (hint->ToString()->Length() == 0)
                {
                    Nan::ThrowError("Hint cannot be an empty string");
                    return false;
                }

                params->hints.push_back(
                    osrm::engine::Hint::FromBase64(*v8::String::Utf8Value(hint)));
            }
            else if (hint->IsNull())
            {
                params->hints.emplace_back();
            }
            else
            {
                Nan::ThrowError("Hint must be null or string");
                return false;
            }
        }
    }

    if (obj->Has(Nan::New("radiuses").ToLocalChecked()))
    {
        v8::Local<v8::Value> radiuses = obj->Get(Nan::New("radiuses").ToLocalChecked());
        if (radiuses.IsEmpty())
            return false;

        if (!radiuses->IsArray())
        {
            Nan::ThrowError("Radiuses must be an array of non-negative doubles or null");
            return false;
        }

        v8::Local<v8::Array> radiuses_array = v8::Local<v8::Array>::Cast(radiuses);

        if (radiuses_array->Length() != params->coordinates.size())
        {
            Nan::ThrowError("Radiuses array must have the same length as coordinates array");
            return false;
        }

        for (uint32_t i = 0; i < radiuses_array->Length(); ++i)
        {
            v8::Local<v8::Value> radius = radiuses_array->Get(i);
            if (radius.IsEmpty())
                return false;

            if (radius->IsNull())
            {
                params->radiuses.emplace_back();
            }
            else if (radius->IsNumber() && radius->NumberValue() >= 0)
            {
                params->radiuses.push_back(static_cast<double>(radius->NumberValue()));
            }
            else
            {
                Nan::ThrowError("Radius must be non-negative double or null");
                return false;
            }
        }
    }

    if (obj->Has(Nan::New("generate_hints").ToLocalChecked()))
    {
        v8::Local<v8::Value> generate_hints = obj->Get(Nan::New("generate_hints").ToLocalChecked());
        if (generate_hints.IsEmpty())
            return false;

        if (!generate_hints->IsBoolean())
        {
            Nan::ThrowError("generate_hints must be of type Boolean");
            return false;
        }

        params->generate_hints = generate_hints->BooleanValue();
    }

    if (obj->Has(Nan::New("exclude").ToLocalChecked()))
    {
        v8::Local<v8::Value> exclude = obj->Get(Nan::New("exclude").ToLocalChecked());
        if (exclude.IsEmpty())
            return false;

        if (!exclude->IsArray())
        {
            Nan::ThrowError("Exclude must be an array of strings or empty");
            return false;
        }

        v8::Local<v8::Array> exclude_array = v8::Local<v8::Array>::Cast(exclude);

        for (uint32_t i = 0; i < exclude_array->Length(); ++i)
        {
            v8::Local<v8::Value> class_name = exclude_array->Get(i);
            if (class_name.IsEmpty())
                return false;

            if (class_name->IsString())
            {
                std::string class_name_str = *v8::String::Utf8Value(class_name);
                params->exclude.emplace_back(class_name_str);
            }
            else
            {
                Nan::ThrowError("Exclude must be an array of strings or empty");
                return false;
            }
        }
    }

    return true;
}

template <typename ParamType>
inline bool parseCommonParameters(const v8::Local<v8::Object> &obj, ParamType &params)
{
    if (obj->Has(Nan::New("steps").ToLocalChecked()))
    {
        auto steps = obj->Get(Nan::New("steps").ToLocalChecked());
        if (steps.IsEmpty())
            return false;

        if (steps->IsBoolean())
        {
            params->steps = steps->BooleanValue();
        }
        else
        {
            Nan::ThrowError("'steps' param must be a boolean");
            return false;
        }
    }

    if (obj->Has(Nan::New("annotations").ToLocalChecked()))
    {
        auto annotations = obj->Get(Nan::New("annotations").ToLocalChecked());
        if (annotations.IsEmpty())
            return false;

        if (annotations->IsBoolean())
        {
            params->annotations = annotations->BooleanValue();
        }
        else if (annotations->IsArray())
        {
            v8::Local<v8::Array> annotations_array = v8::Local<v8::Array>::Cast(annotations);
            for (std::size_t i = 0; i < annotations_array->Length(); i++)
            {
                const Nan::Utf8String annotations_utf8str(annotations_array->Get(i));
                std::string annotations_str{*annotations_utf8str,
                                            *annotations_utf8str + annotations_utf8str.length()};

                if (annotations_str == "duration")
                {
                    params->annotations_type =
                        params->annotations_type | osrm::RouteParameters::AnnotationsType::Duration;
                }
                else if (annotations_str == "nodes")
                {
                    params->annotations_type =
                        params->annotations_type | osrm::RouteParameters::AnnotationsType::Nodes;
                }
                else if (annotations_str == "distance")
                {
                    params->annotations_type =
                        params->annotations_type | osrm::RouteParameters::AnnotationsType::Distance;
                }
                else if (annotations_str == "weight")
                {
                    params->annotations_type =
                        params->annotations_type | osrm::RouteParameters::AnnotationsType::Weight;
                }
                else if (annotations_str == "datasources")
                {
                    params->annotations_type = params->annotations_type |
                                               osrm::RouteParameters::AnnotationsType::Datasources;
                }
                else if (annotations_str == "speed")
                {
                    params->annotations_type =
                        params->annotations_type | osrm::RouteParameters::AnnotationsType::Speed;
                }
                else
                {
                    Nan::ThrowError("this 'annotations' param is not supported");
                    return false;
                }
            }
        }
        else
        {
            Nan::ThrowError("this 'annotations' param is not supported");
            return false;
        }
    }

    if (obj->Has(Nan::New("geometries").ToLocalChecked()))
    {
        v8::Local<v8::Value> geometries = obj->Get(Nan::New("geometries").ToLocalChecked());
        if (geometries.IsEmpty())
            return false;

        if (!geometries->IsString())
        {
            Nan::ThrowError("Geometries must be a string: [polyline, polyline6, geojson]");
            return false;
        }
        const Nan::Utf8String geometries_utf8str(geometries);
        std::string geometries_str{*geometries_utf8str,
                                   *geometries_utf8str + geometries_utf8str.length()};

        if (geometries_str == "polyline")
        {
            params->geometries = osrm::RouteParameters::GeometriesType::Polyline;
        }
        else if (geometries_str == "polyline6")
        {
            params->geometries = osrm::RouteParameters::GeometriesType::Polyline6;
        }
        else if (geometries_str == "geojson")
        {
            params->geometries = osrm::RouteParameters::GeometriesType::GeoJSON;
        }
        else
        {
            Nan::ThrowError("'geometries' param must be one of [polyline, polyline6, geojson]");
            return false;
        }
    }

    if (obj->Has(Nan::New("overview").ToLocalChecked()))
    {
        v8::Local<v8::Value> overview = obj->Get(Nan::New("overview").ToLocalChecked());
        if (overview.IsEmpty())
            return false;

        if (!overview->IsString())
        {
            Nan::ThrowError("Overview must be a string: [simplified, full, false]");
            return false;
        }

        const Nan::Utf8String overview_utf8str(overview);
        std::string overview_str{*overview_utf8str, *overview_utf8str + overview_utf8str.length()};

        if (overview_str == "simplified")
        {
            params->overview = osrm::RouteParameters::OverviewType::Simplified;
        }
        else if (overview_str == "full")
        {
            params->overview = osrm::RouteParameters::OverviewType::Full;
        }
        else if (overview_str == "false")
        {
            params->overview = osrm::RouteParameters::OverviewType::False;
        }
        else
        {
            Nan::ThrowError("'overview' param must be one of [simplified, full, false]");
            return false;
        }
    }

    return true;
}

inline PluginParameters
argumentsToPluginParameters(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    if (args.Length() < 3 || !args[1]->IsObject())
    {
        return {};
    }
    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[1]).ToLocalChecked();
    if (obj->Has(Nan::New("format").ToLocalChecked()))
    {

        v8::Local<v8::Value> format = obj->Get(Nan::New("format").ToLocalChecked());
        if (format.IsEmpty())
        {
            return {};
        }

        if (!format->IsString())
        {
            Nan::ThrowError("format must be a string: \"object\" or \"json_buffer\"");
            return {};
        }

        const Nan::Utf8String format_utf8str(format);
        std::string format_str{*format_utf8str, *format_utf8str + format_utf8str.length()};

        if (format_str == "object")
        {
            return {false};
        }
        else if (format_str == "json_buffer")
        {
            return {true};
        }
        else
        {
            Nan::ThrowError("format must be a string: \"object\" or \"json_buffer\"");
            return {};
        }
    }

    return {};
}

inline route_parameters_ptr
argumentsToRouteParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                          bool requires_multiple_coordinates)
{
    route_parameters_ptr params = std::make_unique<osrm::RouteParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return route_parameters_ptr();

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();

    if (obj->Has(Nan::New("continue_straight").ToLocalChecked()))
    {
        auto value = obj->Get(Nan::New("continue_straight").ToLocalChecked());
        if (value.IsEmpty())
            return route_parameters_ptr();

        if (!value->IsBoolean() && !value->IsNull())
        {
            Nan::ThrowError("'continue_straight' param must be boolean or null");
            return route_parameters_ptr();
        }
        if (value->IsBoolean())
        {
            params->continue_straight = value->BooleanValue();
        }
    }

    if (obj->Has(Nan::New("alternatives").ToLocalChecked()))
    {
        auto value = obj->Get(Nan::New("alternatives").ToLocalChecked());
        if (value.IsEmpty())
            return route_parameters_ptr();

        if (value->IsBoolean())
        {
            params->alternatives = value->BooleanValue();
            params->number_of_alternatives = value->BooleanValue() ? 1u : 0u;
        }
        else if (value->IsNumber())
        {
            params->alternatives = value->BooleanValue();
            params->number_of_alternatives = static_cast<unsigned>(value->NumberValue());
        }
        else
        {
            Nan::ThrowError("'alternatives' param must be boolean or number");
            return route_parameters_ptr();
        }
    }

    if (obj->Has(Nan::New("waypoints").ToLocalChecked()))
    {
        v8::Local<v8::Value> waypoints = obj->Get(Nan::New("waypoints").ToLocalChecked());
        if (waypoints.IsEmpty())
            return route_parameters_ptr();

        // must be array
        if (!waypoints->IsArray())
        {
            Nan::ThrowError(
                "Waypoints must be an array of integers corresponding to the input coordinates.");
            return route_parameters_ptr();
        }

        auto waypoints_array = v8::Local<v8::Array>::Cast(waypoints);
        // must have at least two elements
        if (waypoints_array->Length() < 2)
        {
            Nan::ThrowError("At least two waypoints must be provided");
            return route_parameters_ptr();
        }
        auto coords_size = params->coordinates.size();
        auto waypoints_array_size = waypoints_array->Length();

        const auto first_index = Nan::To<std::uint32_t>(waypoints_array->Get(0)).FromJust();
        const auto last_index =
            Nan::To<std::uint32_t>(waypoints_array->Get(waypoints_array_size - 1)).FromJust();
        if (first_index != 0 || last_index != coords_size - 1)
        {
            Nan::ThrowError("First and last waypoints values must correspond to first and last "
                            "coordinate indices");
            return route_parameters_ptr();
        }

        for (uint32_t i = 0; i < waypoints_array_size; ++i)
        {
            v8::Local<v8::Value> waypoint_value = waypoints_array->Get(i);
            // all elements must be numbers
            if (!waypoint_value->IsNumber())
            {
                Nan::ThrowError("Waypoint values must be an array of integers");
                return route_parameters_ptr();
            }
            // check that the waypoint index corresponds with an inpute coordinate
            const auto index = Nan::To<std::uint32_t>(waypoint_value).FromJust();
            if (index >= coords_size)
            {
                Nan::ThrowError("Waypoints must correspond with the index of an input coordinate");
                return route_parameters_ptr();
            }
            params->waypoints.emplace_back(static_cast<unsigned>(waypoint_value->NumberValue()));
        }

        if (!params->waypoints.empty())
        {
            for (std::size_t i = 0; i < params->waypoints.size() - 1; i++)
            {
                if (params->waypoints[i] >= params->waypoints[i + 1])
                {
                    Nan::ThrowError("Waypoints must be supplied in increasing order");
                    return route_parameters_ptr();
                }
            }
        }
    }

    bool parsedSuccessfully = parseCommonParameters(obj, params);
    if (!parsedSuccessfully)
    {
        return route_parameters_ptr();
    }

    return params;
}

inline tile_parameters_ptr
argumentsToTileParameters(const Nan::FunctionCallbackInfo<v8::Value> &args, bool /*unused*/)
{
    tile_parameters_ptr params = std::make_unique<osrm::TileParameters>();

    if (args.Length() < 2)
    {
        Nan::ThrowTypeError("Coordinate object and callback required");
        return tile_parameters_ptr();
    }

    if (!args[0]->IsArray())
    {
        Nan::ThrowTypeError("Parameter must be an array [x, y, z]");
        return tile_parameters_ptr();
    }

    v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(args[0]);

    if (array->Length() != 3)
    {
        Nan::ThrowTypeError("Parameter must be an array [x, y, z]");
        return tile_parameters_ptr();
    }

    v8::Local<v8::Value> x = array->Get(0);
    v8::Local<v8::Value> y = array->Get(1);
    v8::Local<v8::Value> z = array->Get(2);
    if (x.IsEmpty() || y.IsEmpty() || z.IsEmpty())
        return tile_parameters_ptr();

    if (!x->IsUint32() && !x->IsUndefined())
    {
        Nan::ThrowError("Tile x coordinate must be unsigned interger");
        return tile_parameters_ptr();
    }
    if (!y->IsUint32() && !y->IsUndefined())
    {
        Nan::ThrowError("Tile y coordinate must be unsigned interger");
        return tile_parameters_ptr();
    }
    if (!z->IsUint32() && !z->IsUndefined())
    {
        Nan::ThrowError("Tile z coordinate must be unsigned interger");
        return tile_parameters_ptr();
    }

    params->x = x->Uint32Value();
    params->y = y->Uint32Value();
    params->z = z->Uint32Value();

    if (!params->IsValid())
    {
        Nan::ThrowError("Invalid tile coordinates");
        return tile_parameters_ptr();
    }

    return params;
}

inline nearest_parameters_ptr
argumentsToNearestParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                            bool requires_multiple_coordinates)
{
    nearest_parameters_ptr params = std::make_unique<osrm::NearestParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return nearest_parameters_ptr();

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();
    if (obj.IsEmpty())
        return nearest_parameters_ptr();

    if (obj->Has(Nan::New("number").ToLocalChecked()))
    {
        v8::Local<v8::Value> number = obj->Get(Nan::New("number").ToLocalChecked());

        if (!number->IsUint32())
        {
            Nan::ThrowError("Number must be an integer greater than or equal to 1");
            return nearest_parameters_ptr();
        }
        else
        {
            unsigned number_value = static_cast<unsigned>(number->NumberValue());

            if (number_value < 1)
            {
                Nan::ThrowError("Number must be an integer greater than or equal to 1");
                return nearest_parameters_ptr();
            }

            params->number_of_results = static_cast<unsigned>(number->NumberValue());
        }
    }

    return params;
}

inline table_parameters_ptr
argumentsToTableParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                          bool requires_multiple_coordinates)
{
    table_parameters_ptr params = std::make_unique<osrm::TableParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return table_parameters_ptr();

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();
    if (obj.IsEmpty())
        return table_parameters_ptr();

    if (obj->Has(Nan::New("sources").ToLocalChecked()))
    {
        v8::Local<v8::Value> sources = obj->Get(Nan::New("sources").ToLocalChecked());
        if (sources.IsEmpty())
            return table_parameters_ptr();

        if (!sources->IsArray())
        {
            Nan::ThrowError("Sources must be an array of indices (or undefined)");
            return table_parameters_ptr();
        }

        v8::Local<v8::Array> sources_array = v8::Local<v8::Array>::Cast(sources);
        for (uint32_t i = 0; i < sources_array->Length(); ++i)
        {
            v8::Local<v8::Value> source = sources_array->Get(i);
            if (source.IsEmpty())
                return table_parameters_ptr();

            if (source->IsUint32())
            {
                size_t source_value = static_cast<size_t>(source->NumberValue());
                if (source_value > params->coordinates.size())
                {
                    Nan::ThrowError(
                        "Source indices must be less than or equal to the number of coordinates");
                    return table_parameters_ptr();
                }

                params->sources.push_back(static_cast<size_t>(source->NumberValue()));
            }
            else
            {
                Nan::ThrowError("Source must be an integer");
                return table_parameters_ptr();
            }
        }
    }

    if (obj->Has(Nan::New("destinations").ToLocalChecked()))
    {
        v8::Local<v8::Value> destinations = obj->Get(Nan::New("destinations").ToLocalChecked());
        if (destinations.IsEmpty())
            return table_parameters_ptr();

        if (!destinations->IsArray())
        {
            Nan::ThrowError("Destinations must be an array of indices (or undefined)");
            return table_parameters_ptr();
        }

        v8::Local<v8::Array> destinations_array = v8::Local<v8::Array>::Cast(destinations);
        for (uint32_t i = 0; i < destinations_array->Length(); ++i)
        {
            v8::Local<v8::Value> destination = destinations_array->Get(i);
            if (destination.IsEmpty())
                return table_parameters_ptr();

            if (destination->IsUint32())
            {
                size_t destination_value = static_cast<size_t>(destination->NumberValue());
                if (destination_value > params->coordinates.size())
                {
                    Nan::ThrowError("Destination indices must be less than or equal to the number "
                                    "of coordinates");
                    return table_parameters_ptr();
                }

                params->destinations.push_back(static_cast<size_t>(destination->NumberValue()));
            }
            else
            {
                Nan::ThrowError("Destination must be an integer");
                return table_parameters_ptr();
            }
        }
    }

    if (obj->Has(Nan::New("annotations").ToLocalChecked()))
    {
        v8::Local<v8::Value> annotations = obj->Get(Nan::New("annotations").ToLocalChecked());
        if (annotations.IsEmpty())
            return table_parameters_ptr();

        if (!annotations->IsArray())
        {
            Nan::ThrowError(
                "Annotations must an array containing 'duration' or 'distance', or both");
            return table_parameters_ptr();
        }

        params->annotations = osrm::TableParameters::AnnotationsType::None;

        v8::Local<v8::Array> annotations_array = v8::Local<v8::Array>::Cast(annotations);
        for (std::size_t i = 0; i < annotations_array->Length(); ++i)
        {
            const Nan::Utf8String annotations_utf8str(annotations_array->Get(i));
            std::string annotations_str{*annotations_utf8str,
                                        *annotations_utf8str + annotations_utf8str.length()};

            if (annotations_str == "duration")
            {
                params->annotations =
                    params->annotations | osrm::TableParameters::AnnotationsType::Duration;
            }
            else if (annotations_str == "distance")
            {
                params->annotations =
                    params->annotations | osrm::TableParameters::AnnotationsType::Distance;
            }
            else
            {
                Nan::ThrowError("this 'annotations' param is not supported");
                return table_parameters_ptr();
            }
        }
    }

    if (obj->Has(Nan::New("fallback_speed").ToLocalChecked()))
    {
        auto fallback_speed = obj->Get(Nan::New("fallback_speed").ToLocalChecked());

        if (!fallback_speed->IsNumber())
        {
            Nan::ThrowError("fallback_speed must be a number");
            return table_parameters_ptr();
        }
        else if (fallback_speed->NumberValue() <= 0)
        {
            Nan::ThrowError("fallback_speed must be > 0");
            return table_parameters_ptr();
        }

        params->fallback_speed = static_cast<double>(fallback_speed->NumberValue());
    }

    if (obj->Has(Nan::New("fallback_coordinate").ToLocalChecked()))
    {
        auto fallback_coordinate = obj->Get(Nan::New("fallback_coordinate").ToLocalChecked());

        if (!fallback_coordinate->IsString())
        {
            Nan::ThrowError("fallback_coordinate must be a string: [input, snapped]");
            return table_parameters_ptr();
        }

        std::string fallback_coordinate_str = *v8::String::Utf8Value(fallback_coordinate);

        if (fallback_coordinate_str == "snapped")
        {
            params->fallback_coordinate_type =
                osrm::TableParameters::FallbackCoordinateType::Snapped;
        }
        else if (fallback_coordinate_str == "input")
        {
            params->fallback_coordinate_type = osrm::TableParameters::FallbackCoordinateType::Input;
        }
        else
        {
            Nan::ThrowError("'fallback_coordinate' param must be one of [input, snapped]");
            return table_parameters_ptr();
        }
    }

    if (obj->Has(Nan::New("scale_factor").ToLocalChecked()))
    {
        auto scale_factor = obj->Get(Nan::New("scale_factor").ToLocalChecked());

        if (!scale_factor->IsNumber())
        {
            Nan::ThrowError("scale_factor must be a number");
            return table_parameters_ptr();
        }
        else if (scale_factor->NumberValue() <= 0)
        {
            Nan::ThrowError("scale_factor must be > 0");
            return table_parameters_ptr();
        }

        params->scale_factor = static_cast<double>(scale_factor->NumberValue());
    }

    return params;
}

inline trip_parameters_ptr
argumentsToTripParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                         bool requires_multiple_coordinates)
{
    trip_parameters_ptr params = std::make_unique<osrm::TripParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return trip_parameters_ptr();

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();

    bool parsedSuccessfully = parseCommonParameters(obj, params);
    if (!parsedSuccessfully)
    {
        return trip_parameters_ptr();
    }

    if (obj->Has(Nan::New("roundtrip").ToLocalChecked()))
    {
        auto roundtrip = obj->Get(Nan::New("roundtrip").ToLocalChecked());
        if (roundtrip.IsEmpty())
            return trip_parameters_ptr();

        if (roundtrip->IsBoolean())
        {
            params->roundtrip = roundtrip->BooleanValue();
        }
        else
        {
            Nan::ThrowError("'roundtrip' param must be a boolean");
            return trip_parameters_ptr();
        }
    }

    if (obj->Has(Nan::New("source").ToLocalChecked()))
    {
        v8::Local<v8::Value> source = obj->Get(Nan::New("source").ToLocalChecked());
        if (source.IsEmpty())
            return trip_parameters_ptr();

        if (!source->IsString())
        {
            Nan::ThrowError("Source must be a string: [any, first]");
            return trip_parameters_ptr();
        }

        std::string source_str = *v8::String::Utf8Value(source);

        if (source_str == "first")
        {
            params->source = osrm::TripParameters::SourceType::First;
        }
        else if (source_str == "any")
        {
            params->source = osrm::TripParameters::SourceType::Any;
        }
        else
        {
            Nan::ThrowError("'source' param must be one of [any, first]");
            return trip_parameters_ptr();
        }
    }

    if (obj->Has(Nan::New("destination").ToLocalChecked()))
    {
        v8::Local<v8::Value> destination = obj->Get(Nan::New("destination").ToLocalChecked());
        if (destination.IsEmpty())
            return trip_parameters_ptr();

        if (!destination->IsString())
        {
            Nan::ThrowError("Destination must be a string: [any, last]");
            return trip_parameters_ptr();
        }

        std::string destination_str = *v8::String::Utf8Value(destination);

        if (destination_str == "last")
        {
            params->destination = osrm::TripParameters::DestinationType::Last;
        }
        else if (destination_str == "any")
        {
            params->destination = osrm::TripParameters::DestinationType::Any;
        }
        else
        {
            Nan::ThrowError("'destination' param must be one of [any, last]");
            return trip_parameters_ptr();
        }
    }

    return params;
}

inline match_parameters_ptr
argumentsToMatchParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                          bool requires_multiple_coordinates)
{
    match_parameters_ptr params = std::make_unique<osrm::MatchParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return match_parameters_ptr();

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();

    if (obj->Has(Nan::New("timestamps").ToLocalChecked()))
    {
        v8::Local<v8::Value> timestamps = obj->Get(Nan::New("timestamps").ToLocalChecked());
        if (timestamps.IsEmpty())
            return match_parameters_ptr();

        if (!timestamps->IsArray())
        {
            Nan::ThrowError("Timestamps must be an array of integers (or undefined)");
            return match_parameters_ptr();
        }

        v8::Local<v8::Array> timestamps_array = v8::Local<v8::Array>::Cast(timestamps);

        if (params->coordinates.size() != timestamps_array->Length())
        {
            Nan::ThrowError("Timestamp array must have the same size as the coordinates "
                            "array");
            return match_parameters_ptr();
        }

        for (uint32_t i = 0; i < timestamps_array->Length(); ++i)
        {
            v8::Local<v8::Value> timestamp = timestamps_array->Get(i);
            if (timestamp.IsEmpty())
                return match_parameters_ptr();

            if (!timestamp->IsNumber())
            {
                Nan::ThrowError("Timestamps array items must be numbers");
                return match_parameters_ptr();
            }
            params->timestamps.emplace_back(static_cast<std::size_t>(timestamp->NumberValue()));
        }
    }

    if (obj->Has(Nan::New("gaps").ToLocalChecked()))
    {
        v8::Local<v8::Value> gaps = obj->Get(Nan::New("gaps").ToLocalChecked());
        if (gaps.IsEmpty())
            return match_parameters_ptr();

        if (!gaps->IsString())
        {
            Nan::ThrowError("Gaps must be a string: [split, ignore]");
            return match_parameters_ptr();
        }

        const Nan::Utf8String gaps_utf8str(gaps);
        std::string gaps_str{*gaps_utf8str, *gaps_utf8str + gaps_utf8str.length()};

        if (gaps_str == "split")
        {
            params->gaps = osrm::MatchParameters::GapsType::Split;
        }
        else if (gaps_str == "ignore")
        {
            params->gaps = osrm::MatchParameters::GapsType::Ignore;
        }
        else
        {
            Nan::ThrowError("'gaps' param must be one of [split, ignore]");
            return match_parameters_ptr();
        }
    }

    if (obj->Has(Nan::New("tidy").ToLocalChecked()))
    {
        v8::Local<v8::Value> tidy = obj->Get(Nan::New("tidy").ToLocalChecked());
        if (tidy.IsEmpty())
            return match_parameters_ptr();

        if (!tidy->IsBoolean())
        {
            Nan::ThrowError("tidy must be of type Boolean");
            return match_parameters_ptr();
        }

        params->tidy = tidy->BooleanValue();
    }

    if (obj->Has(Nan::New("waypoints").ToLocalChecked()))
    {
        v8::Local<v8::Value> waypoints = obj->Get(Nan::New("waypoints").ToLocalChecked());
        if (waypoints.IsEmpty())
            return match_parameters_ptr();

        // must be array
        if (!waypoints->IsArray())
        {
            Nan::ThrowError(
                "Waypoints must be an array of integers corresponding to the input coordinates.");
            return match_parameters_ptr();
        }

        auto waypoints_array = v8::Local<v8::Array>::Cast(waypoints);
        // must have at least two elements
        if (waypoints_array->Length() < 2)
        {
            Nan::ThrowError("At least two waypoints must be provided");
            return match_parameters_ptr();
        }
        auto coords_size = params->coordinates.size();
        auto waypoints_array_size = waypoints_array->Length();

        const auto first_index = Nan::To<std::uint32_t>(waypoints_array->Get(0)).FromJust();
        const auto last_index =
            Nan::To<std::uint32_t>(waypoints_array->Get(waypoints_array_size - 1)).FromJust();
        if (first_index != 0 || last_index != coords_size - 1)
        {
            Nan::ThrowError("First and last waypoints values must correspond to first and last "
                            "coordinate indices");
            return match_parameters_ptr();
        }

        for (uint32_t i = 0; i < waypoints_array_size; ++i)
        {
            v8::Local<v8::Value> waypoint_value = waypoints_array->Get(i);
            // all elements must be numbers
            if (!waypoint_value->IsNumber())
            {
                Nan::ThrowError("Waypoint values must be an array of integers");
                return match_parameters_ptr();
            }
            // check that the waypoint index corresponds with an inpute coordinate
            const auto index = Nan::To<std::uint32_t>(waypoint_value).FromJust();
            if (index >= coords_size)
            {
                Nan::ThrowError("Waypoints must correspond with the index of an input coordinate");
                return match_parameters_ptr();
            }
            params->waypoints.emplace_back(static_cast<unsigned>(waypoint_value->NumberValue()));
        }
    }

    bool parsedSuccessfully = parseCommonParameters(obj, params);
    if (!parsedSuccessfully)
    {
        return match_parameters_ptr();
    }

    return params;
}

} // namespace node_osrm

#endif
