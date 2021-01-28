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
        engine_config->storage_config =
            osrm::StorageConfig(*Nan::Utf8String(Nan::To<v8::String>(args[0]).ToLocalChecked()));
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

    auto path = Nan::Get(params, Nan::New("path").ToLocalChecked()).ToLocalChecked();
    if (path.IsEmpty())
        return engine_config_ptr();

    auto memory_file = Nan::Get(params, Nan::New("memory_file").ToLocalChecked()).ToLocalChecked();
    if (memory_file.IsEmpty())
        return engine_config_ptr();

    auto shared_memory =
        Nan::Get(params, Nan::New("shared_memory").ToLocalChecked()).ToLocalChecked();
    if (shared_memory.IsEmpty())
        return engine_config_ptr();

    auto mmap_memory = Nan::Get(params, Nan::New("mmap_memory").ToLocalChecked()).ToLocalChecked();
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
            *Nan::Utf8String(Nan::To<v8::String>(memory_file).ToLocalChecked());
    }

    auto dataset_name =
        Nan::Get(params, Nan::New("dataset_name").ToLocalChecked()).ToLocalChecked();
    if (dataset_name.IsEmpty())
        return engine_config_ptr();
    if (!dataset_name->IsUndefined())
    {
        if (dataset_name->IsString())
        {
            engine_config->dataset_name =
                *Nan::Utf8String(Nan::To<v8::String>(dataset_name).ToLocalChecked());
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
            osrm::StorageConfig(*Nan::Utf8String(Nan::To<v8::String>(path).ToLocalChecked()));

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

    auto algorithm = Nan::Get(params, Nan::New("algorithm").ToLocalChecked()).ToLocalChecked();
    if (algorithm.IsEmpty())
        return engine_config_ptr();

    if (algorithm->IsString())
    {
        auto algorithm_str = Nan::To<v8::String>(algorithm).ToLocalChecked();
        if (*Nan::Utf8String(algorithm_str) == std::string("CH"))
        {
            engine_config->algorithm = osrm::EngineConfig::Algorithm::CH;
        }
        else if (*Nan::Utf8String(algorithm_str) == std::string("CoreCH"))
        {
            engine_config->algorithm = osrm::EngineConfig::Algorithm::CH;
        }
        else if (*Nan::Utf8String(algorithm_str) == std::string("MLD"))
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

    auto max_locations_trip =
        Nan::Get(params, Nan::New("max_locations_trip").ToLocalChecked()).ToLocalChecked();
    auto max_locations_viaroute =
        Nan::Get(params, Nan::New("max_locations_viaroute").ToLocalChecked()).ToLocalChecked();
    auto max_locations_distance_table =
        Nan::Get(params, Nan::New("max_locations_distance_table").ToLocalChecked())
            .ToLocalChecked();
    auto max_locations_map_matching =
        Nan::Get(params, Nan::New("max_locations_map_matching").ToLocalChecked()).ToLocalChecked();
    auto max_results_nearest =
        Nan::Get(params, Nan::New("max_results_nearest").ToLocalChecked()).ToLocalChecked();
    auto max_alternatives =
        Nan::Get(params, Nan::New("max_alternatives").ToLocalChecked()).ToLocalChecked();
    auto max_radius_map_matching =
        Nan::Get(params, Nan::New("max_radius_map_matching").ToLocalChecked()).ToLocalChecked();

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
        engine_config->max_locations_trip = Nan::To<int>(max_locations_trip).FromJust();
    if (max_locations_viaroute->IsNumber())
        engine_config->max_locations_viaroute = Nan::To<int>(max_locations_viaroute).FromJust();
    if (max_locations_distance_table->IsNumber())
        engine_config->max_locations_distance_table =
            Nan::To<int>(max_locations_distance_table).FromJust();
    if (max_locations_map_matching->IsNumber())
        engine_config->max_locations_map_matching =
            Nan::To<int>(max_locations_map_matching).FromJust();
    if (max_results_nearest->IsNumber())
        engine_config->max_results_nearest = Nan::To<int>(max_results_nearest).FromJust();
    if (max_alternatives->IsNumber())
        engine_config->max_alternatives = Nan::To<int>(max_alternatives).FromJust();
    if (max_radius_map_matching->IsNumber())
        engine_config->max_radius_map_matching =
            Nan::To<double>(max_radius_map_matching).FromJust();

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
        v8::Local<v8::Value> coordinate = Nan::Get(coordinates_array, i).ToLocalChecked();
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

        if (!Nan::Get(coordinate_pair, 0).ToLocalChecked()->IsNumber() ||
            !Nan::Get(coordinate_pair, 1).ToLocalChecked()->IsNumber())
        {
            Nan::ThrowError("Each member of a coordinate pair must be a number");
            return resulting_coordinates;
        }

        double lon = Nan::To<double>(Nan::Get(coordinate_pair, 0).ToLocalChecked()).FromJust();
        double lat = Nan::To<double>(Nan::Get(coordinate_pair, 1).ToLocalChecked()).FromJust();

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

    v8::Local<v8::Value> coordinates =
        Nan::Get(obj, Nan::New("coordinates").ToLocalChecked()).ToLocalChecked();
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

    if (Nan::Has(obj, Nan::New("approaches").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> approaches =
            Nan::Get(obj, Nan::New("approaches").ToLocalChecked()).ToLocalChecked();
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
            v8::Local<v8::Value> approach_raw = Nan::Get(approaches_array, i).ToLocalChecked();
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

    if (Nan::Has(obj, Nan::New("bearings").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> bearings =
            Nan::Get(obj, Nan::New("bearings").ToLocalChecked()).ToLocalChecked();
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
            v8::Local<v8::Value> bearing_raw = Nan::Get(bearings_array, i).ToLocalChecked();
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
                    if (!Nan::Get(bearing_pair, 0).ToLocalChecked()->IsNumber() ||
                        !Nan::Get(bearing_pair, 1).ToLocalChecked()->IsNumber())
                    {
                        Nan::ThrowError("Bearing values need to be numbers in range 0..360");
                        return false;
                    }

                    const auto bearing =
                        Nan::To<int>(Nan::Get(bearing_pair, 0).ToLocalChecked()).FromJust();
                    const auto range =
                        Nan::To<int>(Nan::Get(bearing_pair, 1).ToLocalChecked()).FromJust();

                    if (bearing < 0 || bearing > 360 || range < 0 || range > 180)
                    {
                        Nan::ThrowError("Bearing values need to be in range 0..360, 0..180");
                        return false;
                    }

                    params->bearings.push_back(
                        osrm::Bearing{static_cast<short>(bearing), static_cast<short>(range)});
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

    if (Nan::Has(obj, Nan::New("hints").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> hints =
            Nan::Get(obj, Nan::New("hints").ToLocalChecked()).ToLocalChecked();
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
            v8::Local<v8::Value> hint = Nan::Get(hints_array, i).ToLocalChecked();
            if (hint.IsEmpty())
                return false;

            if (hint->IsString())
            {
                if (Nan::To<v8::String>(hint).ToLocalChecked()->Length() == 0)
                {
                    Nan::ThrowError("Hint cannot be an empty string");
                    return false;
                }

                params->hints.push_back(osrm::engine::Hint::FromBase64(*Nan::Utf8String(hint)));
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

    if (Nan::Has(obj, Nan::New("radiuses").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> radiuses =
            Nan::Get(obj, Nan::New("radiuses").ToLocalChecked()).ToLocalChecked();
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
            v8::Local<v8::Value> radius = Nan::Get(radiuses_array, i).ToLocalChecked();
            if (radius.IsEmpty())
                return false;

            if (radius->IsNull())
            {
                params->radiuses.emplace_back();
            }
            else if (radius->IsNumber() && Nan::To<double>(radius).FromJust() >= 0)
            {
                params->radiuses.push_back(Nan::To<double>(radius).FromJust());
            }
            else
            {
                Nan::ThrowError("Radius must be non-negative double or null");
                return false;
            }
        }
    }

    if (Nan::Has(obj, Nan::New("generate_hints").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> generate_hints =
            Nan::Get(obj, Nan::New("generate_hints").ToLocalChecked()).ToLocalChecked();
        if (generate_hints.IsEmpty())
            return false;

        if (!generate_hints->IsBoolean())
        {
            Nan::ThrowError("generate_hints must be of type Boolean");
            return false;
        }

        params->generate_hints = Nan::To<bool>(generate_hints).FromJust();
    }

    if (Nan::Has(obj, Nan::New("exclude").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> exclude =
            Nan::Get(obj, Nan::New("exclude").ToLocalChecked()).ToLocalChecked();
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
            v8::Local<v8::Value> class_name = Nan::Get(exclude_array, i).ToLocalChecked();
            if (class_name.IsEmpty())
                return false;

            if (class_name->IsString())
            {
                std::string class_name_str = *Nan::Utf8String(class_name);
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
    if (Nan::Has(obj, Nan::New("steps").ToLocalChecked()).FromJust())
    {
        auto steps = Nan::Get(obj, Nan::New("steps").ToLocalChecked()).ToLocalChecked();
        if (steps.IsEmpty())
            return false;

        if (steps->IsBoolean())
        {
            params->steps = Nan::To<bool>(steps).FromJust();
        }
        else
        {
            Nan::ThrowError("'steps' param must be a boolean");
            return false;
        }
    }

    if (Nan::Has(obj, Nan::New("annotations").ToLocalChecked()).FromJust())
    {
        auto annotations = Nan::Get(obj, Nan::New("annotations").ToLocalChecked()).ToLocalChecked();
        if (annotations.IsEmpty())
            return false;

        if (annotations->IsBoolean())
        {
            params->annotations = Nan::To<bool>(annotations).FromJust();
        }
        else if (annotations->IsArray())
        {
            v8::Local<v8::Array> annotations_array = v8::Local<v8::Array>::Cast(annotations);
            for (std::size_t i = 0; i < annotations_array->Length(); i++)
            {
                const Nan::Utf8String annotations_utf8str(
                    Nan::Get(annotations_array, i).ToLocalChecked());
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

    if (Nan::Has(obj, Nan::New("geometries").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> geometries =
            Nan::Get(obj, Nan::New("geometries").ToLocalChecked()).ToLocalChecked();
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

    if (Nan::Has(obj, Nan::New("overview").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> overview =
            Nan::Get(obj, Nan::New("overview").ToLocalChecked()).ToLocalChecked();
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
    if (Nan::Has(obj, Nan::New("format").ToLocalChecked()).FromJust())
    {

        v8::Local<v8::Value> format =
            Nan::Get(obj, Nan::New("format").ToLocalChecked()).ToLocalChecked();
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

    if (Nan::Has(obj, Nan::New("continue_straight").ToLocalChecked()).FromJust())
    {
        auto value = Nan::Get(obj, Nan::New("continue_straight").ToLocalChecked()).ToLocalChecked();
        if (value.IsEmpty())
            return route_parameters_ptr();

        if (!value->IsBoolean() && !value->IsNull())
        {
            Nan::ThrowError("'continue_straight' param must be boolean or null");
            return route_parameters_ptr();
        }
        if (value->IsBoolean())
        {
            params->continue_straight = Nan::To<bool>(value).FromJust();
        }
    }

    if (Nan::Has(obj, Nan::New("alternatives").ToLocalChecked()).FromJust())
    {
        auto value = Nan::Get(obj, Nan::New("alternatives").ToLocalChecked()).ToLocalChecked();
        if (value.IsEmpty())
            return route_parameters_ptr();

        if (value->IsBoolean())
        {
            params->alternatives = Nan::To<bool>(value).FromJust();
            params->number_of_alternatives = Nan::To<bool>(value).FromJust() ? 1u : 0u;
        }
        else if (value->IsNumber())
        {
            params->alternatives = Nan::To<bool>(value).FromJust();
            params->number_of_alternatives = Nan::To<unsigned>(value).FromJust();
        }
        else
        {
            Nan::ThrowError("'alternatives' param must be boolean or number");
            return route_parameters_ptr();
        }
    }

    if (Nan::Has(obj, Nan::New("waypoints").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> waypoints =
            Nan::Get(obj, Nan::New("waypoints").ToLocalChecked()).ToLocalChecked();
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

        const auto first_index =
            Nan::To<std::uint32_t>(Nan::Get(waypoints_array, 0).ToLocalChecked()).FromJust();
        const auto last_index =
            Nan::To<std::uint32_t>(
                Nan::Get(waypoints_array, waypoints_array_size - 1).ToLocalChecked())
                .FromJust();
        if (first_index != 0 || last_index != coords_size - 1)
        {
            Nan::ThrowError("First and last waypoints values must correspond to first and last "
                            "coordinate indices");
            return route_parameters_ptr();
        }

        for (uint32_t i = 0; i < waypoints_array_size; ++i)
        {
            v8::Local<v8::Value> waypoint_value = Nan::Get(waypoints_array, i).ToLocalChecked();
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
            params->waypoints.emplace_back(Nan::To<unsigned>(waypoint_value).FromJust());
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

    if (Nan::Has(obj, Nan::New("snapping").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> snapping =
            Nan::Get(obj, Nan::New("snapping").ToLocalChecked()).ToLocalChecked();
        if (snapping.IsEmpty())
            return route_parameters_ptr();

        if (!snapping->IsString())
        {
            Nan::ThrowError("Snapping must be a string: [default, any]");
            return route_parameters_ptr();
        }
        const Nan::Utf8String snapping_utf8str(snapping);
        std::string snapping_str{*snapping_utf8str, *snapping_utf8str + snapping_utf8str.length()};

        if (snapping_str == "default")
        {
            params->snapping = osrm::RouteParameters::SnappingType::Default;
        }
        else if (snapping_str == "any")
        {
            params->snapping = osrm::RouteParameters::SnappingType::Any;
        }
        else
        {
            Nan::ThrowError("'snapping' param must be one of [default, any]");
            return route_parameters_ptr();
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

    v8::Local<v8::Value> x = Nan::Get(array, 0).ToLocalChecked();
    v8::Local<v8::Value> y = Nan::Get(array, 1).ToLocalChecked();
    v8::Local<v8::Value> z = Nan::Get(array, 2).ToLocalChecked();
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

    params->x = Nan::To<uint32_t>(x).FromJust();
    params->y = Nan::To<uint32_t>(y).FromJust();
    params->z = Nan::To<uint32_t>(z).FromJust();

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

    if (Nan::Has(obj, Nan::New("number").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> number =
            Nan::Get(obj, Nan::New("number").ToLocalChecked()).ToLocalChecked();

        if (!number->IsUint32())
        {
            Nan::ThrowError("Number must be an integer greater than or equal to 1");
            return nearest_parameters_ptr();
        }
        else
        {
            unsigned number_value = Nan::To<unsigned>(number).FromJust();

            if (number_value < 1)
            {
                Nan::ThrowError("Number must be an integer greater than or equal to 1");
                return nearest_parameters_ptr();
            }

            params->number_of_results = Nan::To<unsigned>(number).FromJust();
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

    if (Nan::Has(obj, Nan::New("sources").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> sources =
            Nan::Get(obj, Nan::New("sources").ToLocalChecked()).ToLocalChecked();
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
            v8::Local<v8::Value> source = Nan::Get(sources_array, i).ToLocalChecked();
            if (source.IsEmpty())
                return table_parameters_ptr();

            if (source->IsUint32())
            {
                size_t source_value = Nan::To<unsigned>(source).FromJust();
                if (source_value >= params->coordinates.size())
                {
                    Nan::ThrowError("Source indices must be less than the number of coordinates");
                    return table_parameters_ptr();
                }

                params->sources.push_back(Nan::To<unsigned>(source).FromJust());
            }
            else
            {
                Nan::ThrowError("Source must be an integer");
                return table_parameters_ptr();
            }
        }
    }

    if (Nan::Has(obj, Nan::New("destinations").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> destinations =
            Nan::Get(obj, Nan::New("destinations").ToLocalChecked()).ToLocalChecked();
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
            v8::Local<v8::Value> destination = Nan::Get(destinations_array, i).ToLocalChecked();
            if (destination.IsEmpty())
                return table_parameters_ptr();

            if (destination->IsUint32())
            {
                size_t destination_value = Nan::To<unsigned>(destination).FromJust();
                if (destination_value >= params->coordinates.size())
                {
                    Nan::ThrowError("Destination indices must be less than the number "
                                    "of coordinates");
                    return table_parameters_ptr();
                }

                params->destinations.push_back(Nan::To<unsigned>(destination).FromJust());
            }
            else
            {
                Nan::ThrowError("Destination must be an integer");
                return table_parameters_ptr();
            }
        }
    }

    if (Nan::Has(obj, Nan::New("annotations").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> annotations =
            Nan::Get(obj, Nan::New("annotations").ToLocalChecked()).ToLocalChecked();
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
            const Nan::Utf8String annotations_utf8str(
                Nan::Get(annotations_array, i).ToLocalChecked());
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

    if (Nan::Has(obj, Nan::New("fallback_speed").ToLocalChecked()).FromJust())
    {
        auto fallback_speed =
            Nan::Get(obj, Nan::New("fallback_speed").ToLocalChecked()).ToLocalChecked();

        if (!fallback_speed->IsNumber())
        {
            Nan::ThrowError("fallback_speed must be a number");
            return table_parameters_ptr();
        }
        else if (Nan::To<double>(fallback_speed).FromJust() <= 0)
        {
            Nan::ThrowError("fallback_speed must be > 0");
            return table_parameters_ptr();
        }

        params->fallback_speed = Nan::To<double>(fallback_speed).FromJust();
    }

    if (Nan::Has(obj, Nan::New("fallback_coordinate").ToLocalChecked()).FromJust())
    {
        auto fallback_coordinate =
            Nan::Get(obj, Nan::New("fallback_coordinate").ToLocalChecked()).ToLocalChecked();

        if (!fallback_coordinate->IsString())
        {
            Nan::ThrowError("fallback_coordinate must be a string: [input, snapped]");
            return table_parameters_ptr();
        }

        std::string fallback_coordinate_str = *Nan::Utf8String(fallback_coordinate);

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

    if (Nan::Has(obj, Nan::New("scale_factor").ToLocalChecked()).FromJust())
    {
        auto scale_factor =
            Nan::Get(obj, Nan::New("scale_factor").ToLocalChecked()).ToLocalChecked();

        if (!scale_factor->IsNumber())
        {
            Nan::ThrowError("scale_factor must be a number");
            return table_parameters_ptr();
        }
        else if (Nan::To<double>(scale_factor).FromJust() <= 0)
        {
            Nan::ThrowError("scale_factor must be > 0");
            return table_parameters_ptr();
        }

        params->scale_factor = Nan::To<double>(scale_factor).FromJust();
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

    if (Nan::Has(obj, Nan::New("roundtrip").ToLocalChecked()).FromJust())
    {
        auto roundtrip = Nan::Get(obj, Nan::New("roundtrip").ToLocalChecked()).ToLocalChecked();
        if (roundtrip.IsEmpty())
            return trip_parameters_ptr();

        if (roundtrip->IsBoolean())
        {
            params->roundtrip = Nan::To<bool>(roundtrip).FromJust();
        }
        else
        {
            Nan::ThrowError("'roundtrip' param must be a boolean");
            return trip_parameters_ptr();
        }
    }

    if (Nan::Has(obj, Nan::New("source").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> source =
            Nan::Get(obj, Nan::New("source").ToLocalChecked()).ToLocalChecked();
        if (source.IsEmpty())
            return trip_parameters_ptr();

        if (!source->IsString())
        {
            Nan::ThrowError("Source must be a string: [any, first]");
            return trip_parameters_ptr();
        }

        std::string source_str = *Nan::Utf8String(source);

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

    if (Nan::Has(obj, Nan::New("destination").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> destination =
            Nan::Get(obj, Nan::New("destination").ToLocalChecked()).ToLocalChecked();
        if (destination.IsEmpty())
            return trip_parameters_ptr();

        if (!destination->IsString())
        {
            Nan::ThrowError("Destination must be a string: [any, last]");
            return trip_parameters_ptr();
        }

        std::string destination_str = *Nan::Utf8String(destination);

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

    if (Nan::Has(obj, Nan::New("timestamps").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> timestamps =
            Nan::Get(obj, Nan::New("timestamps").ToLocalChecked()).ToLocalChecked();
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
            v8::Local<v8::Value> timestamp = Nan::Get(timestamps_array, i).ToLocalChecked();
            if (timestamp.IsEmpty())
                return match_parameters_ptr();

            if (!timestamp->IsNumber())
            {
                Nan::ThrowError("Timestamps array items must be numbers");
                return match_parameters_ptr();
            }
            params->timestamps.emplace_back(Nan::To<unsigned>(timestamp).FromJust());
        }
    }

    if (Nan::Has(obj, Nan::New("gaps").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> gaps =
            Nan::Get(obj, Nan::New("gaps").ToLocalChecked()).ToLocalChecked();
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

    if (Nan::Has(obj, Nan::New("tidy").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> tidy =
            Nan::Get(obj, Nan::New("tidy").ToLocalChecked()).ToLocalChecked();
        if (tidy.IsEmpty())
            return match_parameters_ptr();

        if (!tidy->IsBoolean())
        {
            Nan::ThrowError("tidy must be of type Boolean");
            return match_parameters_ptr();
        }

        params->tidy = Nan::To<bool>(tidy).FromJust();
    }

    if (Nan::Has(obj, Nan::New("waypoints").ToLocalChecked()).FromJust())
    {
        v8::Local<v8::Value> waypoints =
            Nan::Get(obj, Nan::New("waypoints").ToLocalChecked()).ToLocalChecked();
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

        const auto first_index =
            Nan::To<std::uint32_t>(Nan::Get(waypoints_array, 0).ToLocalChecked()).FromJust();
        const auto last_index =
            Nan::To<std::uint32_t>(
                Nan::Get(waypoints_array, waypoints_array_size - 1).ToLocalChecked())
                .FromJust();
        if (first_index != 0 || last_index != coords_size - 1)
        {
            Nan::ThrowError("First and last waypoints values must correspond to first and last "
                            "coordinate indices");
            return match_parameters_ptr();
        }

        for (uint32_t i = 0; i < waypoints_array_size; ++i)
        {
            v8::Local<v8::Value> waypoint_value = Nan::Get(waypoints_array, i).ToLocalChecked();
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
            params->waypoints.emplace_back(
                static_cast<unsigned>(Nan::To<unsigned>(waypoint_value).FromJust()));
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
