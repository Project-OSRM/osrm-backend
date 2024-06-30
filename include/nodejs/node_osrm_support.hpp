#ifndef OSRM_BINDINGS_NODE_SUPPORT_HPP
#define OSRM_BINDINGS_NODE_SUPPORT_HPP

#include "nodejs/json_v8_renderer.hpp"
#include "engine/api/flatbuffers/fbresult_generated.h"
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
#include "util/json_renderer.hpp"
#include <napi.h>

#include <boost/assert.hpp>
#include <optional>

#include <algorithm>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
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
    bool renderToBuffer = false;
};

using ObjectOrString = typename std::variant<osrm::json::Object, std::string>;

template <typename ResultT> inline Napi::Value render(const Napi::Env &env, const ResultT &result);

template <> Napi::Value inline render(const Napi::Env &env, const std::string &result)
{
    return Napi::Buffer<char>::Copy(env, result.data(), result.size());
}

template <> Napi::Value inline render(const Napi::Env &env, const ObjectOrString &result)
{
    if (std::holds_alternative<osrm::json::Object>(result))
    {
        // Convert osrm::json object tree into matching v8 object tree
        Napi::Value value;
        renderToV8(env, value, std::get<osrm::json::Object>(result));
        return value;
    }
    else
    {
        // Return the string object as a node Buffer
        return Napi::Buffer<char>::Copy(
            env, std::get<std::string>(result).data(), std::get<std::string>(result).size());
    }
}

inline bool IsUnsignedInteger(const Napi::Value &value)
{
    if (!value.IsNumber())
    {
        return false;
    }
    const auto doubleValue = value.ToNumber().DoubleValue();
    return doubleValue >= 0.0 && std::floor(doubleValue) == doubleValue;
}

inline void ParseResult(const osrm::Status &result_status, osrm::json::Object &result)
{
    const auto code_iter = result.values.find("code");
    const auto end_iter = result.values.end();

    BOOST_ASSERT(code_iter != end_iter);

    if (result_status == osrm::Status::Error)
    {
        throw std::logic_error(std::get<osrm::json::String>(code_iter->second).value.c_str());
    }

    result.values.erase(code_iter);
    const auto message_iter = result.values.find("message");
    if (message_iter != end_iter)
    {
        result.values.erase(message_iter);
    }
}

inline void ParseResult(const osrm::Status & /*result_status*/, const std::string & /*unused*/) {}
inline void ParseResult(const osrm::Status &result_status,
                        const flatbuffers::FlatBufferBuilder &fbs_builder)
{
    auto fbs_result = osrm::engine::api::fbresult::GetFBResult(fbs_builder.GetBufferPointer());

    if (result_status == osrm::Status::Error)
    {
        BOOST_ASSERT(fbs_result->code());
        throw std::logic_error(fbs_result->code()->message()->c_str());
    }
}

inline void ThrowError(const Napi::Env &env, const char *message)
{
    Napi::Error::New(env, message).ThrowAsJavaScriptException();
}

inline void ThrowTypeError(const Napi::Env &env, const char *message)
{
    Napi::TypeError::New(env, message).ThrowAsJavaScriptException();
}

inline engine_config_ptr argumentsToEngineConfig(const Napi::CallbackInfo &args)
{
    Napi::HandleScope scope(args.Env());
    auto engine_config = std::make_unique<osrm::EngineConfig>();

    if (args.Length() == 0)
    {
        return engine_config;
    }
    else if (args.Length() > 1)
    {
        ThrowError(args.Env(), "Only accepts one parameter");
        return engine_config_ptr();
    }

    BOOST_ASSERT(args.Length() == 1);

    if (args[0].IsString())
    {
        engine_config->storage_config = osrm::StorageConfig(args[0].ToString().Utf8Value());
        engine_config->use_shared_memory = false;
        return engine_config;
    }
    else if (!args[0].IsObject())
    {
        ThrowError(args.Env(), "Parameter must be a path or options object");
        return engine_config_ptr();
    }

    BOOST_ASSERT(args[0].IsObject());
    auto params = args[0].As<Napi::Object>();

    auto path = params.Get("path");
    if (path.IsEmpty())
        return engine_config_ptr();

    auto memory_file = params.Get("memory_file");
    if (memory_file.IsEmpty())
        return engine_config_ptr();

    auto shared_memory = params.Get("shared_memory");
    if (shared_memory.IsEmpty())
        return engine_config_ptr();

    auto mmap_memory = params.Get("mmap_memory");
    if (mmap_memory.IsEmpty())
        return engine_config_ptr();

    if (!memory_file.IsUndefined())
    {
        if (path.IsUndefined())
        {
            ThrowError(args.Env(), "memory_file option requires a path to a file.");
            return engine_config_ptr();
        }

        engine_config->memory_file = memory_file.ToString().Utf8Value();
    }

    auto dataset_name = params.Get("dataset_name");
    if (dataset_name.IsEmpty())
        return engine_config_ptr();
    if (!dataset_name.IsUndefined())
    {
        if (dataset_name.IsString())
        {
            engine_config->dataset_name = dataset_name.ToString().Utf8Value();
        }
        else
        {
            ThrowError(args.Env(), "dataset_name needs to be a string");
            return engine_config_ptr();
        }
    }

    auto disable_feature_dataset = params.Get("disable_feature_dataset");
    if (disable_feature_dataset.IsArray())
    {
        Napi::Array datasets = disable_feature_dataset.As<Napi::Array>();
        for (uint32_t i = 0; i < datasets.Length(); ++i)
        {
            Napi::Value dataset = datasets.Get(i);
            if (!dataset.IsString())
            {
                ThrowError(args.Env(), "disable_feature_dataset list option must be a string");
                return engine_config_ptr();
            }
            auto dataset_str = dataset.ToString().Utf8Value();
            if (dataset_str == "ROUTE_GEOMETRY")
            {
                engine_config->disable_feature_dataset.push_back(
                    osrm::storage::FeatureDataset::ROUTE_GEOMETRY);
            }
            else if (dataset_str == "ROUTE_STEPS")
            {
                engine_config->disable_feature_dataset.push_back(
                    osrm::storage::FeatureDataset::ROUTE_STEPS);
            }
            else
            {
                ThrowError(
                    args.Env(),
                    "disable_feature_dataset array can include 'ROUTE_GEOMETRY', 'ROUTE_STEPS'.");
                return engine_config_ptr();
            }
        }
    }
    else if (!disable_feature_dataset.IsUndefined())
    {
        ThrowError(args.Env(),
                   "disable_feature_dataset option must be an array and can include the string "
                   "values 'ROUTE_GEOMETRY', 'ROUTE_STEPS'.");
        return engine_config_ptr();
    }

    if (!path.IsUndefined())
    {
        engine_config->storage_config = osrm::StorageConfig(path.ToString().Utf8Value(),
                                                            engine_config->disable_feature_dataset);

        engine_config->use_shared_memory = false;
    }
    if (!shared_memory.IsUndefined())
    {
        if (shared_memory.IsBoolean())
        {
            engine_config->use_shared_memory = shared_memory.ToBoolean().Value();
        }
        else
        {
            ThrowError(args.Env(), "Shared_memory option must be a boolean");
            return engine_config_ptr();
        }
    }
    if (!mmap_memory.IsUndefined())
    {
        if (mmap_memory.IsBoolean())
        {
            engine_config->use_mmap = mmap_memory.ToBoolean().Value();
        }
        else
        {
            ThrowError(args.Env(), "mmap_memory option must be a boolean");
            return engine_config_ptr();
        }
    }

    if (path.IsUndefined() && !engine_config->use_shared_memory)
    {
        ThrowError(args.Env(),
                   "Shared_memory must be enabled if no path is "
                   "specified");
        return engine_config_ptr();
    }

    auto algorithm = params.Get("algorithm");
    if (algorithm.IsEmpty())
        return engine_config_ptr();

    if (algorithm.IsString())
    {
        auto algorithm_str = algorithm.ToString().Utf8Value();
        if (algorithm_str == "CH")
        {
            engine_config->algorithm = osrm::EngineConfig::Algorithm::CH;
        }
        else if (algorithm_str == "MLD")
        {
            engine_config->algorithm = osrm::EngineConfig::Algorithm::MLD;
        }
        else
        {
            ThrowError(args.Env(), "algorithm option must be one of 'CH', or 'MLD'.");
            return engine_config_ptr();
        }
    }
    else if (!algorithm.IsUndefined())
    {
        ThrowError(args.Env(), "algorithm option must be a string and one of 'CH', or 'MLD'.");
        return engine_config_ptr();
    }

    // Set EngineConfig system-wide limits on construction, if requested

    auto max_locations_trip = params.Get("max_locations_trip");
    auto max_locations_viaroute = params.Get("max_locations_viaroute");
    auto max_locations_distance_table = params.Get("max_locations_distance_table");
    auto max_locations_map_matching = params.Get("max_locations_map_matching");
    auto max_results_nearest = params.Get("max_results_nearest");
    auto max_alternatives = params.Get("max_alternatives");
    auto max_radius_map_matching = params.Get("max_radius_map_matching");
    auto default_radius = params.Get("default_radius");

    if (!max_locations_trip.IsUndefined() && !max_locations_trip.IsNumber())
    {
        ThrowError(args.Env(), "max_locations_trip must be an integral number");
        return engine_config_ptr();
    }
    if (!max_locations_viaroute.IsUndefined() && !max_locations_viaroute.IsNumber())
    {
        ThrowError(args.Env(), "max_locations_viaroute must be an integral number");
        return engine_config_ptr();
    }
    if (!max_locations_distance_table.IsUndefined() && !max_locations_distance_table.IsNumber())
    {
        ThrowError(args.Env(), "max_locations_distance_table must be an integral number");
        return engine_config_ptr();
    }
    if (!max_locations_map_matching.IsUndefined() && !max_locations_map_matching.IsNumber())
    {
        ThrowError(args.Env(), "max_locations_map_matching must be an integral number");
        return engine_config_ptr();
    }
    if (!max_results_nearest.IsUndefined() && !max_results_nearest.IsNumber())
    {
        ThrowError(args.Env(), "max_results_nearest must be an integral number");
        return engine_config_ptr();
    }
    if (!max_alternatives.IsUndefined() && !max_alternatives.IsNumber())
    {
        ThrowError(args.Env(), "max_alternatives must be an integral number");
        return engine_config_ptr();
    }
    if (!max_radius_map_matching.IsUndefined() && max_radius_map_matching.IsString() &&
        max_radius_map_matching.ToString().Utf8Value() != "unlimited")
    {
        ThrowError(args.Env(), "max_radius_map_matching must be unlimited or an integral number");
        return engine_config_ptr();
    }
    if (!default_radius.IsUndefined() && default_radius.IsString() &&
        default_radius.ToString().Utf8Value() != "unlimited")
    {
        ThrowError(args.Env(), "default_radius must be unlimited or an integral number");
        return engine_config_ptr();
    }

    if (max_locations_trip.IsNumber())
        engine_config->max_locations_trip = max_locations_trip.ToNumber().Int32Value();
    if (max_locations_viaroute.IsNumber())
        engine_config->max_locations_viaroute = max_locations_viaroute.ToNumber().Int32Value();
    if (max_locations_distance_table.IsNumber())
        engine_config->max_locations_distance_table =
            max_locations_distance_table.ToNumber().Int32Value();
    if (max_locations_map_matching.IsNumber())
        engine_config->max_locations_map_matching =
            max_locations_map_matching.ToNumber().Int32Value();
    if (max_results_nearest.IsNumber())
        engine_config->max_results_nearest = max_results_nearest.ToNumber().Int32Value();
    if (max_alternatives.IsNumber())
        engine_config->max_alternatives = max_alternatives.ToNumber().Int32Value();

    if (max_radius_map_matching.IsNumber())
        engine_config->max_radius_map_matching = max_radius_map_matching.ToNumber().DoubleValue();
    else if (max_radius_map_matching.IsString() &&
             max_radius_map_matching.ToString().Utf8Value() == "unlimited")
        engine_config->max_radius_map_matching = -1.0;

    if (default_radius.IsNumber())
        engine_config->default_radius = default_radius.ToNumber().DoubleValue();
    else if (default_radius.IsString() && default_radius.ToString().Utf8Value() == "unlimited")
        engine_config->default_radius = -1.0;

    return engine_config;
}

inline std::optional<std::vector<osrm::Coordinate>>
parseCoordinateArray(const Napi::Array &coordinates_array)
{
    Napi::HandleScope scope(coordinates_array.Env());
    std::optional<std::vector<osrm::Coordinate>> resulting_coordinates;
    std::vector<osrm::Coordinate> temp_coordinates;

    for (uint32_t i = 0; i < coordinates_array.Length(); ++i)
    {
        Napi::Value coordinate = coordinates_array.Get(i);
        if (coordinate.IsEmpty())
            return resulting_coordinates;

        if (!coordinate.IsArray())
        {
            ThrowError(coordinates_array.Env(), "Coordinates must be an array of (lon/lat) pairs");
            return resulting_coordinates;
        }

        Napi::Array coordinate_pair = coordinate.As<Napi::Array>();
        if (coordinate_pair.Length() != 2)
        {
            ThrowError(coordinates_array.Env(), "Coordinates must be an array of (lon/lat) pairs");
            return resulting_coordinates;
        }

        if (!coordinate_pair.Get(static_cast<uint32_t>(0)).IsNumber() ||
            !coordinate_pair.Get(static_cast<uint32_t>(1)).IsNumber())
        {
            ThrowError(coordinates_array.Env(),
                       "Each member of a coordinate pair must be a number");
            return resulting_coordinates;
        }

        double lon = coordinate_pair.Get(static_cast<uint32_t>(0)).As<Napi::Number>().DoubleValue();
        double lat = coordinate_pair.Get(static_cast<uint32_t>(1)).As<Napi::Number>().DoubleValue();

        if (std::isnan(lon) || std::isnan(lat) || std::isinf(lon) || std::isinf(lat))
        {
            ThrowError(coordinates_array.Env(), "Lng/Lat coordinates must be valid numbers");
            return resulting_coordinates;
        }

        if (lon > 180 || lon < -180 || lat > 90 || lat < -90)
        {
            ThrowError(coordinates_array.Env(),
                       "Lng/Lat coordinates must be within world bounds "
                       "(-180 < lng < 180, -90 < lat < 90)");
            return resulting_coordinates;
        }

        temp_coordinates.emplace_back(osrm::util::FloatLongitude{std::move(lon)},
                                      osrm::util::FloatLatitude{std::move(lat)});
    }

    resulting_coordinates = std::make_optional(std::move(temp_coordinates));
    return resulting_coordinates;
}

// Parses all the non-service specific parameters
template <typename ParamType>
inline bool argumentsToParameter(const Napi::CallbackInfo &args,
                                 ParamType &params,
                                 bool requires_multiple_coordinates)
{
    Napi::HandleScope scope(args.Env());

    if (args.Length() < 2)
    {
        ThrowTypeError(args.Env(), "Two arguments required");
        return false;
    }

    if (!args[0].IsObject())
    {
        ThrowTypeError(args.Env(), "First arg must be an object");
        return false;
    }

    Napi::Object obj = args[0].As<Napi::Object>();

    Napi::Value coordinates = obj.Get("coordinates");
    if (coordinates.IsEmpty())
        return false;

    if (coordinates.IsUndefined())
    {
        ThrowError(args.Env(), "Must provide a coordinates property");
        return false;
    }
    else if (coordinates.IsArray())
    {
        auto coordinates_array = coordinates.As<Napi::Array>();
        if (coordinates_array.Length() < 2 && requires_multiple_coordinates)
        {
            ThrowError(args.Env(), "At least two coordinates must be provided");
            return false;
        }
        else if (!requires_multiple_coordinates && coordinates_array.Length() != 1)
        {
            ThrowError(args.Env(), "Exactly one coordinate pair must be provided");
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
    else if (!coordinates.IsUndefined())
    {
        BOOST_ASSERT(!coordinates.IsArray());
        ThrowError(args.Env(), "Coordinates must be an array of (lon/lat) pairs");
        return false;
    }

    if (obj.Has("approaches"))
    {
        Napi::Value approaches = obj.Get("approaches");
        if (approaches.IsEmpty())
            return false;

        if (!approaches.IsArray())
        {
            ThrowError(args.Env(), "Approaches must be an arrays of strings");
            return false;
        }

        auto approaches_array = approaches.As<Napi::Array>();

        if (approaches_array.Length() != params->coordinates.size())
        {
            ThrowError(args.Env(),
                       "Approaches array must have the same length as coordinates array");
            return false;
        }

        for (uint32_t i = 0; i < approaches_array.Length(); ++i)
        {
            Napi::Value approach_raw = approaches_array.Get(i);
            if (approach_raw.IsEmpty())
                return false;

            if (approach_raw.IsNull())
            {
                params->approaches.emplace_back();
            }
            else if (approach_raw.IsString())
            {
                std::string approach_str = approach_raw.ToString().Utf8Value();
                if (approach_str == "curb")
                {
                    params->approaches.push_back(osrm::Approach::CURB);
                }
                else if (approach_str == "opposite")
                {
                    params->approaches.push_back(osrm::Approach::OPPOSITE);
                }
                else if (approach_str == "unrestricted")
                {
                    params->approaches.push_back(osrm::Approach::UNRESTRICTED);
                }
                else
                {
                    ThrowError(args.Env(),
                               "'approaches' param must be one of [curb, opposite, unrestricted]");
                    return false;
                }
            }
            else
            {
                ThrowError(args.Env(),
                           "Approach must be a string: [curb, opposite, unrestricted] or null");
                return false;
            }
        }
    }

    if (obj.Has("bearings"))
    {
        Napi::Value bearings = obj.Get("bearings");
        if (bearings.IsEmpty())
            return false;

        if (!bearings.IsArray())
        {
            ThrowError(args.Env(), "Bearings must be an array of arrays of numbers");
            return false;
        }

        auto bearings_array = bearings.As<Napi::Array>();

        if (bearings_array.Length() != params->coordinates.size())
        {
            ThrowError(args.Env(), "Bearings array must have the same length as coordinates array");
            return false;
        }

        for (uint32_t i = 0; i < bearings_array.Length(); ++i)
        {
            Napi::Value bearing_raw = bearings_array.Get(i);
            if (bearing_raw.IsEmpty())
                return false;

            if (bearing_raw.IsNull())
            {
                params->bearings.emplace_back();
            }
            else if (bearing_raw.IsArray())
            {
                auto bearing_pair = bearing_raw.As<Napi::Array>();
                if (bearing_pair.Length() == 2)
                {
                    if (!bearing_pair.Get(static_cast<uint32_t>(0)).IsNumber() ||
                        !bearing_pair.Get(static_cast<uint32_t>(1)).IsNumber())
                    {
                        ThrowError(args.Env(), "Bearing values need to be numbers in range 0..360");
                        return false;
                    }

                    const auto bearing =
                        bearing_pair.Get(static_cast<uint32_t>(0)).ToNumber().Int32Value();
                    const auto range =
                        bearing_pair.Get(static_cast<uint32_t>(1)).ToNumber().Int32Value();

                    if (bearing < 0 || bearing > 360 || range < 0 || range > 180)
                    {
                        ThrowError(args.Env(), "Bearing values need to be in range 0..360, 0..180");
                        return false;
                    }

                    params->bearings.push_back(
                        osrm::Bearing{static_cast<short>(bearing), static_cast<short>(range)});
                }
                else
                {
                    ThrowError(args.Env(), "Bearing must be an array of [bearing, range] or null");
                    return false;
                }
            }
            else
            {
                ThrowError(args.Env(), "Bearing must be an array of [bearing, range] or null");
                return false;
            }
        }
    }

    if (obj.Has("hints"))
    {
        Napi::Value hints = obj.Get("hints");
        if (hints.IsEmpty())
            return false;

        if (!hints.IsArray())
        {
            ThrowError(args.Env(), "Hints must be an array of strings/null");
            return false;
        }

        Napi::Array hints_array = hints.As<Napi::Array>();

        if (hints_array.Length() != params->coordinates.size())
        {
            ThrowError(args.Env(), "Hints array must have the same length as coordinates array");
            return false;
        }

        for (uint32_t i = 0; i < hints_array.Length(); ++i)
        {
            Napi::Value hint = hints_array.Get(i);
            if (hint.IsEmpty())
                return false;

            if (hint.IsString())
            {
                if (hint.ToString().Utf8Value().length() == 0)
                {
                    ThrowError(args.Env(), "Hint cannot be an empty string");
                    return false;
                }

                params->hints.emplace_back(
                    osrm::engine::Hint::FromBase64(hint.ToString().Utf8Value()));
            }
            else if (hint.IsNull())
            {
                params->hints.emplace_back();
            }
            else
            {
                ThrowError(args.Env(), "Hint must be null or string");
                return false;
            }
        }
    }

    if (obj.Has("radiuses"))
    {
        Napi::Value radiuses = obj.Get("radiuses");
        if (radiuses.IsEmpty())
            return false;

        if (!radiuses.IsArray())
        {
            ThrowError(args.Env(), "Radiuses must be an array of non-negative doubles or null");
            return false;
        }

        Napi::Array radiuses_array = radiuses.As<Napi::Array>();

        if (radiuses_array.Length() != params->coordinates.size())
        {
            ThrowError(args.Env(), "Radiuses array must have the same length as coordinates array");
            return false;
        }

        for (uint32_t i = 0; i < radiuses_array.Length(); ++i)
        {
            Napi::Value radius = radiuses_array.Get(i);
            if (radius.IsEmpty())
                return false;

            if (radius.IsNull())
            {
                params->radiuses.emplace_back();
            }
            else if (radius.IsNumber() && radius.ToNumber().DoubleValue() >= 0)
            {
                params->radiuses.push_back(radius.ToNumber().DoubleValue());
            }
            else
            {
                ThrowError(args.Env(), "Radius must be non-negative double or null");
                return false;
            }
        }
    }

    if (obj.Has("generate_hints"))
    {
        Napi::Value generate_hints = obj.Get("generate_hints");
        if (generate_hints.IsEmpty())
            return false;

        if (!generate_hints.IsBoolean())
        {
            ThrowError(args.Env(), "generate_hints must be of type Boolean");
            return false;
        }

        params->generate_hints = generate_hints.ToBoolean().Value();
    }

    if (obj.Has("skip_waypoints"))
    {
        Napi::Value skip_waypoints = obj.Get("skip_waypoints");
        if (skip_waypoints.IsEmpty())
            return false;

        if (!skip_waypoints.IsBoolean())
        {
            ThrowError(args.Env(), "skip_waypoints must be of type Boolean");
            return false;
        }

        params->skip_waypoints = skip_waypoints.ToBoolean().Value();
    }

    if (obj.Has("exclude"))
    {
        Napi::Value exclude = obj.Get("exclude");
        if (exclude.IsEmpty())
            return false;

        if (!exclude.IsArray())
        {
            ThrowError(args.Env(), "Exclude must be an array of strings or empty");
            return false;
        }

        Napi::Array exclude_array = exclude.As<Napi::Array>();

        for (uint32_t i = 0; i < exclude_array.Length(); ++i)
        {
            Napi::Value class_name = exclude_array.Get(i);
            if (class_name.IsEmpty())
                return false;

            if (class_name.IsString())
            {
                std::string class_name_str = class_name.ToString().Utf8Value();
                params->exclude.emplace_back(std::move(class_name_str));
            }
            else
            {
                ThrowError(args.Env(), "Exclude must be an array of strings or empty");
                return false;
            }
        }
    }

    if (obj.Has("format"))
    {
        Napi::Value format = obj.Get("format");
        if (format.IsEmpty())
        {
            return false;
        }

        if (!format.IsString())
        {
            ThrowError(args.Env(), "format must be a string: \"json\" or \"flatbuffers\"");
            return false;
        }

        std::string format_str = format.ToString().Utf8Value();
        if (format_str == "json")
        {
            params->format = osrm::engine::api::BaseParameters::OutputFormatType::JSON;
        }
        else if (format_str == "flatbuffers")
        {
            params->format = osrm::engine::api::BaseParameters::OutputFormatType::FLATBUFFERS;
        }
        else
        {
            ThrowError(args.Env(), "format must be a string: \"json\" or \"flatbuffers\"");
            return false;
        }
    }

    if (obj.Has("snapping"))
    {
        Napi::Value snapping = obj.Get("snapping");
        if (snapping.IsEmpty())
            return false;

        if (!snapping.IsString())
        {
            ThrowError(args.Env(), "Snapping must be a string: [default, any]");
            return false;
        }

        std::string snapping_str = snapping.ToString().Utf8Value();

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
            ThrowError(args.Env(), "'snapping' param must be one of [default, any]");
            return false;
        }
    }

    return true;
}

template <typename ParamType>
inline bool parseCommonParameters(const Napi::Object &obj, ParamType &params)
{
    if (obj.Has("steps"))
    {
        auto steps = obj.Get("steps");
        if (steps.IsEmpty())
            return false;

        if (steps.IsBoolean())
        {
            params->steps = steps.ToBoolean().Value();
        }
        else
        {
            ThrowError(obj.Env(), "'steps' param must be a boolean");
            return false;
        }
    }

    if (obj.Has("annotations"))
    {
        auto annotations = obj.Get("annotations");
        if (annotations.IsEmpty())
            return false;

        if (annotations.IsBoolean())
        {
            params->annotations = annotations.ToBoolean().Value();
            params->annotations_type = params->annotations
                                           ? osrm::RouteParameters::AnnotationsType::All
                                           : osrm::RouteParameters::AnnotationsType::None;
        }
        else if (annotations.IsArray())
        {
            Napi::Array annotations_array = annotations.As<Napi::Array>();
            for (std::size_t i = 0; i < annotations_array.Length(); i++)
            {
                std::string annotations_str = annotations_array.Get(i).ToString().Utf8Value();

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
                    ThrowError(obj.Env(), "this 'annotations' param is not supported");
                    return false;
                }

                params->annotations =
                    params->annotations_type != osrm::RouteParameters::AnnotationsType::None;
            }
        }
        else
        {
            ThrowError(obj.Env(), "this 'annotations' param is not supported");
            return false;
        }
    }

    if (obj.Has("geometries"))
    {
        Napi::Value geometries = obj.Get("geometries");
        if (geometries.IsEmpty())
            return false;

        if (!geometries.IsString())
        {
            ThrowError(obj.Env(), "Geometries must be a string: [polyline, polyline6, geojson]");
            return false;
        }
        std::string geometries_str = geometries.ToString().Utf8Value();

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
            ThrowError(obj.Env(),
                       "'geometries' param must be one of [polyline, polyline6, geojson]");
            return false;
        }
    }

    if (obj.Has("overview"))
    {
        Napi::Value overview = obj.Get("overview");
        if (overview.IsEmpty())
            return false;

        if (!overview.IsString())
        {
            ThrowError(obj.Env(), "Overview must be a string: [simplified, full, false]");
            return false;
        }

        std::string overview_str = overview.ToString().Utf8Value();

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
            ThrowError(obj.Env(), "'overview' param must be one of [simplified, full, false]");
            return false;
        }
    }

    return true;
}

inline PluginParameters argumentsToPluginParameters(
    const Napi::CallbackInfo &args,
    const std::optional<osrm::engine::api::BaseParameters::OutputFormatType> &output_format = {})
{
    if (args.Length() < 3 || !args[1].IsObject())
    {
        // output to buffer by default for Flatbuffers
        return {output_format == osrm::engine::api::BaseParameters::OutputFormatType::FLATBUFFERS};
    }
    Napi::Object obj = args[1].As<Napi::Object>();
    if (obj.Has("format"))
    {
        Napi::Value format = obj.Get("format");
        if (format.IsEmpty())
        {
            return {};
        }

        if (!format.IsString())
        {
            ThrowError(args.Env(), "format must be a string: \"object\" or \"buffer\"");
            return {};
        }

        std::string format_str = format.ToString().Utf8Value();

        if (format_str == "object")
        {
            if (output_format == osrm::engine::api::BaseParameters::OutputFormatType::FLATBUFFERS)
            {
                ThrowError(args.Env(), "Flatbuffers result can only output to buffer.");
                return {true};
            }
            return {false};
        }
        else if (format_str == "buffer")
        {
            return {true};
        }
        else if (format_str == "json_buffer")
        {
            if (output_format &&
                output_format != osrm::engine::api::BaseParameters::OutputFormatType::JSON)
            {
                ThrowError(args.Env(),
                           "Deprecated `json_buffer` can only be used with JSON format");
            }
            return {true};
        }
        else
        {
            ThrowError(args.Env(), "format must be a string: \"object\" or \"buffer\"");
            return {};
        }
    }

    // output to buffer by default for Flatbuffers
    return {output_format == osrm::engine::api::BaseParameters::OutputFormatType::FLATBUFFERS};
}

inline route_parameters_ptr argumentsToRouteParameter(const Napi::CallbackInfo &args,
                                                      bool requires_multiple_coordinates)
{
    route_parameters_ptr params = std::make_unique<osrm::RouteParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return route_parameters_ptr();

    Napi::Object obj = args[0].As<Napi::Object>();

    if (obj.Has("continue_straight"))
    {
        auto value = obj.Get("continue_straight");
        if (value.IsEmpty())
            return route_parameters_ptr();

        if (!value.IsBoolean() && !value.IsNull())
        {
            ThrowError(args.Env(), "'continue_straight' param must be boolean or null");
            return route_parameters_ptr();
        }
        if (value.IsBoolean())
        {
            params->continue_straight = value.ToBoolean().Value();
        }
    }

    if (obj.Has("alternatives"))
    {
        auto value = obj.Get("alternatives");
        if (value.IsEmpty())
            return route_parameters_ptr();

        if (value.IsBoolean())
        {
            params->alternatives = value.ToBoolean().Value();
            params->number_of_alternatives = value.ToBoolean().Value() ? 1u : 0u;
        }
        else if (value.IsNumber())
        {
            params->alternatives = value.ToBoolean().Value();
            params->number_of_alternatives = value.ToNumber().Int32Value();
        }
        else
        {
            ThrowError(args.Env(), "'alternatives' param must be boolean or number");
            return route_parameters_ptr();
        }
    }

    if (obj.Has("waypoints"))
    {
        Napi::Value waypoints = obj.Get("waypoints");
        if (waypoints.IsEmpty())
            return route_parameters_ptr();

        // must be array
        if (!waypoints.IsArray())
        {
            ThrowError(
                args.Env(),
                "Waypoints must be an array of integers corresponding to the input coordinates.");
            return route_parameters_ptr();
        }

        auto waypoints_array = waypoints.As<Napi::Array>();
        // must have at least two elements
        if (waypoints_array.Length() < 2)
        {
            ThrowError(args.Env(), "At least two waypoints must be provided");
            return route_parameters_ptr();
        }
        auto coords_size = params->coordinates.size();
        auto waypoints_array_size = waypoints_array.Length();

        const auto first_index =
            waypoints_array.Get(static_cast<uint32_t>(0)).ToNumber().Uint32Value();
        const auto last_index =
            waypoints_array.Get(waypoints_array_size - 1).ToNumber().Uint32Value();
        if (first_index != 0 || last_index != coords_size - 1)
        {
            ThrowError(args.Env(),
                       "First and last waypoints values must correspond to first and last "
                       "coordinate indices");
            return route_parameters_ptr();
        }

        for (uint32_t i = 0; i < waypoints_array_size; ++i)
        {
            Napi::Value waypoint_value = waypoints_array.Get(i);
            // all elements must be numbers
            if (!waypoint_value.IsNumber())
            {
                ThrowError(args.Env(), "Waypoint values must be an array of integers");
                return route_parameters_ptr();
            }
            // check that the waypoint index corresponds with an inpute coordinate
            const auto index = waypoint_value.ToNumber().Uint32Value();
            if (index >= coords_size)
            {
                ThrowError(args.Env(),
                           "Waypoints must correspond with the index of an input coordinate");
                return route_parameters_ptr();
            }
            params->waypoints.emplace_back(index);
        }

        if (!params->waypoints.empty())
        {
            for (std::size_t i = 0; i < params->waypoints.size() - 1; i++)
            {
                if (params->waypoints[i] >= params->waypoints[i + 1])
                {
                    ThrowError(args.Env(), "Waypoints must be supplied in increasing order");
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

inline tile_parameters_ptr argumentsToTileParameters(const Napi::CallbackInfo &args,
                                                     bool /*unused*/)
{
    tile_parameters_ptr params = std::make_unique<osrm::TileParameters>();

    if (args.Length() < 2)
    {
        ThrowTypeError(args.Env(), "Coordinate object and callback required");
        return tile_parameters_ptr();
    }

    if (!args[0].IsArray())
    {
        ThrowTypeError(args.Env(), "Parameter must be an array [x, y, z]");
        return tile_parameters_ptr();
    }

    Napi::Array array = args[0].As<Napi::Array>();

    if (array.Length() != 3)
    {
        ThrowTypeError(args.Env(), "Parameter must be an array [x, y, z]");
        return tile_parameters_ptr();
    }

    Napi::Value x = array.Get(static_cast<uint32_t>(0));
    Napi::Value y = array.Get(static_cast<uint32_t>(1));
    Napi::Value z = array.Get(static_cast<uint32_t>(2));
    if (x.IsEmpty() || y.IsEmpty() || z.IsEmpty())
        return tile_parameters_ptr();

    if (!IsUnsignedInteger(x) && !x.IsUndefined())
    {
        ThrowError(args.Env(), "Tile x coordinate must be unsigned interger");
        return tile_parameters_ptr();
    }
    if (!IsUnsignedInteger(y) && !y.IsUndefined())
    {
        ThrowError(args.Env(), "Tile y coordinate must be unsigned interger");
        return tile_parameters_ptr();
    }
    if (!IsUnsignedInteger(z) && !z.IsUndefined())
    {
        ThrowError(args.Env(), "Tile z coordinate must be unsigned interger");
        return tile_parameters_ptr();
    }

    params->x = x.ToNumber().Uint32Value();
    params->y = y.ToNumber().Uint32Value();
    params->z = z.ToNumber().Uint32Value();

    if (!params->IsValid())
    {
        ThrowError(args.Env(), "Invalid tile coordinates");
        return tile_parameters_ptr();
    }

    return params;
}

inline nearest_parameters_ptr argumentsToNearestParameter(const Napi::CallbackInfo &args,
                                                          bool requires_multiple_coordinates)
{
    nearest_parameters_ptr params = std::make_unique<osrm::NearestParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return nearest_parameters_ptr();

    Napi::Object obj = args[0].As<Napi::Object>();
    if (obj.IsEmpty())
        return nearest_parameters_ptr();

    if (obj.Has("number"))
    {
        Napi::Value number = obj.Get("number");

        if (!IsUnsignedInteger(number))
        {
            ThrowError(args.Env(), "Number must be an integer greater than or equal to 1");
            return nearest_parameters_ptr();
        }
        else
        {
            unsigned number_value = number.ToNumber().Uint32Value();

            if (number_value < 1)
            {
                ThrowError(args.Env(), "Number must be an integer greater than or equal to 1");
                return nearest_parameters_ptr();
            }

            params->number_of_results = number_value;
        }
    }

    return params;
}

inline table_parameters_ptr argumentsToTableParameter(const Napi::CallbackInfo &args,
                                                      bool requires_multiple_coordinates)
{
    table_parameters_ptr params = std::make_unique<osrm::TableParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return table_parameters_ptr();

    Napi::Object obj = args[0].As<Napi::Object>();
    if (obj.IsEmpty())
        return table_parameters_ptr();

    if (obj.Has("sources"))
    {
        Napi::Value sources = obj.Get("sources");
        if (sources.IsEmpty())
            return table_parameters_ptr();

        if (!sources.IsArray())
        {
            ThrowError(args.Env(), "Sources must be an array of indices (or undefined)");
            return table_parameters_ptr();
        }

        Napi::Array sources_array = sources.As<Napi::Array>();
        for (uint32_t i = 0; i < sources_array.Length(); ++i)
        {
            Napi::Value source = sources_array.Get(i);
            if (source.IsEmpty())
                return table_parameters_ptr();

            if (IsUnsignedInteger(source))
            {
                size_t source_value = source.ToNumber().Uint32Value();
                if (source_value >= params->coordinates.size())
                {
                    ThrowError(args.Env(),
                               "Source indices must be less than the number of coordinates");
                    return table_parameters_ptr();
                }

                params->sources.push_back(source.ToNumber().Uint32Value());
            }
            else
            {
                ThrowError(args.Env(), "Source must be an integer");
                return table_parameters_ptr();
            }
        }
    }

    if (obj.Has("destinations"))
    {
        Napi::Value destinations = obj.Get("destinations");
        if (destinations.IsEmpty())
            return table_parameters_ptr();

        if (!destinations.IsArray())
        {
            ThrowError(args.Env(), "Destinations must be an array of indices (or undefined)");
            return table_parameters_ptr();
        }

        Napi::Array destinations_array = destinations.As<Napi::Array>();
        for (uint32_t i = 0; i < destinations_array.Length(); ++i)
        {
            Napi::Value destination = destinations_array.Get(i);
            if (destination.IsEmpty())
                return table_parameters_ptr();

            if (IsUnsignedInteger(destination))
            {
                size_t destination_value = destination.ToNumber().Uint32Value();
                if (destination_value >= params->coordinates.size())
                {
                    ThrowError(args.Env(),
                               "Destination indices must be less than the number "
                               "of coordinates");
                    return table_parameters_ptr();
                }

                params->destinations.push_back(destination_value);
            }
            else
            {
                ThrowError(args.Env(), "Destination must be an integer");
                return table_parameters_ptr();
            }
        }
    }

    if (obj.Has("annotations"))
    {
        Napi::Value annotations = obj.Get("annotations");
        if (annotations.IsEmpty())
            return table_parameters_ptr();

        if (!annotations.IsArray())
        {
            ThrowError(args.Env(),
                       "Annotations must an array containing 'duration' or 'distance', or both");
            return table_parameters_ptr();
        }

        params->annotations = osrm::TableParameters::AnnotationsType::None;

        Napi::Array annotations_array = annotations.As<Napi::Array>();
        for (std::size_t i = 0; i < annotations_array.Length(); ++i)
        {
            std::string annotations_str = annotations_array.Get(i).ToString().Utf8Value();

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
                ThrowError(args.Env(), "this 'annotations' param is not supported");
                return table_parameters_ptr();
            }
        }
    }

    if (obj.Has("fallback_speed"))
    {
        auto fallback_speed = obj.Get("fallback_speed");

        if (!fallback_speed.IsNumber())
        {
            ThrowError(args.Env(), "fallback_speed must be a number");
            return table_parameters_ptr();
        }
        else if (fallback_speed.ToNumber().DoubleValue() <= 0)
        {
            ThrowError(args.Env(), "fallback_speed must be > 0");
            return table_parameters_ptr();
        }

        params->fallback_speed = fallback_speed.ToNumber().DoubleValue();
    }

    if (obj.Has("fallback_coordinate"))
    {
        auto fallback_coordinate = obj.Get("fallback_coordinate");

        if (!fallback_coordinate.IsString())
        {
            ThrowError(args.Env(), "fallback_coordinate must be a string: [input, snapped]");
            return table_parameters_ptr();
        }

        std::string fallback_coordinate_str = fallback_coordinate.ToString().Utf8Value();

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
            ThrowError(args.Env(), "'fallback_coordinate' param must be one of [input, snapped]");
            return table_parameters_ptr();
        }
    }

    if (obj.Has("scale_factor"))
    {
        auto scale_factor = obj.Get("scale_factor");

        if (!scale_factor.IsNumber())
        {
            ThrowError(args.Env(), "scale_factor must be a number");
            return table_parameters_ptr();
        }
        else if (scale_factor.ToNumber().DoubleValue() <= 0)
        {
            ThrowError(args.Env(), "scale_factor must be > 0");
            return table_parameters_ptr();
        }

        params->scale_factor = scale_factor.ToNumber().DoubleValue();
    }

    return params;
}

inline trip_parameters_ptr argumentsToTripParameter(const Napi::CallbackInfo &args,
                                                    bool requires_multiple_coordinates)
{
    trip_parameters_ptr params = std::make_unique<osrm::TripParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return trip_parameters_ptr();

    Napi::Object obj = args[0].As<Napi::Object>();

    bool parsedSuccessfully = parseCommonParameters(obj, params);
    if (!parsedSuccessfully)
    {
        return trip_parameters_ptr();
    }

    if (obj.Has("roundtrip"))
    {
        auto roundtrip = obj.Get("roundtrip");
        if (roundtrip.IsEmpty())
            return trip_parameters_ptr();

        if (roundtrip.IsBoolean())
        {
            params->roundtrip = roundtrip.ToBoolean().Value();
        }
        else
        {
            ThrowError(args.Env(), "'roundtrip' param must be a boolean");
            return trip_parameters_ptr();
        }
    }

    if (obj.Has("source"))
    {
        Napi::Value source = obj.Get("source");
        if (source.IsEmpty())
            return trip_parameters_ptr();

        if (!source.IsString())
        {
            ThrowError(args.Env(), "Source must be a string: [any, first]");
            return trip_parameters_ptr();
        }

        std::string source_str = source.ToString().Utf8Value();

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
            ThrowError(args.Env(), "'source' param must be one of [any, first]");
            return trip_parameters_ptr();
        }
    }

    if (obj.Has("destination"))
    {
        Napi::Value destination = obj.Get("destination");
        if (destination.IsEmpty())
            return trip_parameters_ptr();

        if (!destination.IsString())
        {
            ThrowError(args.Env(), "Destination must be a string: [any, last]");
            return trip_parameters_ptr();
        }

        std::string destination_str = destination.ToString().Utf8Value();

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
            ThrowError(args.Env(), "'destination' param must be one of [any, last]");
            return trip_parameters_ptr();
        }
    }

    return params;
}

inline match_parameters_ptr argumentsToMatchParameter(const Napi::CallbackInfo &args,
                                                      bool requires_multiple_coordinates)
{
    match_parameters_ptr params = std::make_unique<osrm::MatchParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return match_parameters_ptr();

    Napi::Object obj = args[0].As<Napi::Object>();

    if (obj.Has("timestamps"))
    {
        Napi::Value timestamps = obj.Get("timestamps");
        if (timestamps.IsEmpty())
            return match_parameters_ptr();

        if (!timestamps.IsArray())
        {
            ThrowError(args.Env(), "Timestamps must be an array of integers (or undefined)");
            return match_parameters_ptr();
        }

        Napi::Array timestamps_array = timestamps.As<Napi::Array>();

        if (params->coordinates.size() != timestamps_array.Length())
        {
            ThrowError(args.Env(),
                       "Timestamp array must have the same size as the coordinates "
                       "array");
            return match_parameters_ptr();
        }

        for (uint32_t i = 0; i < timestamps_array.Length(); ++i)
        {
            Napi::Value timestamp = timestamps_array.Get(i);
            if (timestamp.IsEmpty())
                return match_parameters_ptr();

            if (!timestamp.IsNumber())
            {
                ThrowError(args.Env(), "Timestamps array items must be numbers");
                return match_parameters_ptr();
            }
            params->timestamps.emplace_back(timestamp.ToNumber().Int64Value());
        }
    }

    if (obj.Has("gaps"))
    {
        Napi::Value gaps = obj.Get("gaps");
        if (gaps.IsEmpty())
            return match_parameters_ptr();

        if (!gaps.IsString())
        {
            ThrowError(args.Env(), "Gaps must be a string: [split, ignore]");
            return match_parameters_ptr();
        }

        std::string gaps_str = gaps.ToString().Utf8Value();

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
            ThrowError(args.Env(), "'gaps' param must be one of [split, ignore]");
            return match_parameters_ptr();
        }
    }

    if (obj.Has("tidy"))
    {
        Napi::Value tidy = obj.Get("tidy");
        if (tidy.IsEmpty())
            return match_parameters_ptr();

        if (!tidy.IsBoolean())
        {
            ThrowError(args.Env(), "tidy must be of type Boolean");
            return match_parameters_ptr();
        }

        params->tidy = tidy.ToBoolean().Value();
    }

    if (obj.Has("waypoints"))
    {
        Napi::Value waypoints = obj.Get("waypoints");
        if (waypoints.IsEmpty())
            return match_parameters_ptr();

        // must be array
        if (!waypoints.IsArray())
        {
            ThrowError(
                args.Env(),
                "Waypoints must be an array of integers corresponding to the input coordinates.");
            return match_parameters_ptr();
        }

        auto waypoints_array = waypoints.As<Napi::Array>();
        // must have at least two elements
        if (waypoints_array.Length() < 2)
        {
            ThrowError(args.Env(), "At least two waypoints must be provided");
            return match_parameters_ptr();
        }
        auto coords_size = params->coordinates.size();
        auto waypoints_array_size = waypoints_array.Length();

        const auto first_index =
            waypoints_array.Get(static_cast<uint32_t>(0)).ToNumber().Uint32Value();
        const auto last_index =
            waypoints_array.Get(waypoints_array_size - 1).ToNumber().Uint32Value();
        if (first_index != 0 || last_index != coords_size - 1)
        {
            ThrowError(args.Env(),
                       "First and last waypoints values must correspond to first and last "
                       "coordinate indices");
            return match_parameters_ptr();
        }

        for (uint32_t i = 0; i < waypoints_array_size; ++i)
        {
            Napi::Value waypoint_value = waypoints_array.Get(i);
            // all elements must be numbers
            if (!waypoint_value.IsNumber())
            {
                ThrowError(args.Env(), "Waypoint values must be an array of integers");
                return match_parameters_ptr();
            }
            // check that the waypoint index corresponds with an inpute coordinate
            const auto index = waypoint_value.ToNumber().Uint32Value();
            if (index >= coords_size)
            {
                ThrowError(args.Env(),
                           "Waypoints must correspond with the index of an input coordinate");
                return match_parameters_ptr();
            }
            params->waypoints.emplace_back(index);
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
