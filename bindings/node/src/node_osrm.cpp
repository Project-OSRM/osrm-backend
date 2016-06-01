#include "json_v8_renderer.hpp"

// v8
#include <nan.h>

// OSRM
#include <osrm/json_container.hpp>
#include <osrm/engine_config.hpp>
#include <osrm/storage_config.hpp>
#include <osrm/osrm.hpp>
#include <osrm/route_parameters.hpp>
#include <osrm/table_parameters.hpp>
#include <osrm/nearest_parameters.hpp>
#include <osrm/match_parameters.hpp>
#include <osrm/trip_parameters.hpp>
#include <osrm/tile_parameters.hpp>
#include <osrm/bearing.hpp>
#include <osrm/coordinate.hpp>

#include <boost/optional.hpp>
#include <boost/assert.hpp>

// STL
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <iterator>
#include <utility>
#include <exception>
#include <string>

namespace node_osrm
{

using osrm_ptr = std::unique_ptr<osrm::OSRM>;
using engine_config_ptr = std::unique_ptr<osrm::EngineConfig>;
using route_parameters_ptr = std::unique_ptr<osrm::RouteParameters>;
using trip_parameters_ptr = std::unique_ptr<osrm::TripParameters>;
using tile_parameters_ptr = std::unique_ptr<osrm::TileParameters>;
using match_parameters_ptr = std::unique_ptr<osrm::MatchParameters>;
using nearest_parameters_ptr = std::unique_ptr<osrm::NearestParameters>;
using table_parameters_ptr = std::unique_ptr<osrm::TableParameters>;
namespace
{
template <class T, class... Types> std::unique_ptr<T> make_unique(Types &&... Args)
{
    return (std::unique_ptr<T>(new T(std::forward<Types>(Args)...)));
}
}

// Supports
engine_config_ptr argumentsToEngineConfig(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    Nan::HandleScope scope;
    auto engine_config = make_unique<osrm::EngineConfig>();

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
    auto shared_memory = params->Get(Nan::New("shared_memory").ToLocalChecked());
    if (!path->IsUndefined())
    {
        engine_config->storage_config = osrm::StorageConfig(
            *v8::String::Utf8Value(Nan::To<v8::String>(path).ToLocalChecked()));
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

    if (path->IsUndefined() && !engine_config->use_shared_memory)
    {
        Nan::ThrowError("Shared_memory must be enabled if no path is "
                        "specified");
        return engine_config_ptr();
    }

    return engine_config;
}

boost::optional<std::vector<osrm::Coordinate>>
parseCoordinateArray(const v8::Local<v8::Array> &coordinates_array)
{
    Nan::HandleScope scope;
    boost::optional<std::vector<osrm::Coordinate>> resulting_coordinates;
    std::vector<osrm::Coordinate> temp_coordinates;

    for (uint32_t i = 0; i < coordinates_array->Length(); ++i)
    {
        v8::Local<v8::Value> coordinate = coordinates_array->Get(i);

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

        temp_coordinates.emplace_back(osrm::util::FloatLongitude(std::move(lon)),
                                      osrm::util::FloatLatitude(std::move(lat)));
    }

    resulting_coordinates = boost::make_optional(std::move(temp_coordinates));
    return resulting_coordinates;
}

// Parses all the non-service specific parameters
template <typename ParamType>
bool argumentsToParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
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
            std::copy(maybe_coordinates->begin(), maybe_coordinates->end(),
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

    if (obj->Has(Nan::New("bearings").ToLocalChecked()))
    {
        v8::Local<v8::Value> bearings = obj->Get(Nan::New("bearings").ToLocalChecked());

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

                    short bearing_first = static_cast<short>(bearing_pair->Get(0)->NumberValue());
                    short bearing_second = static_cast<short>(bearing_pair->Get(1)->NumberValue());

                    if (bearing_first < 0 || bearing_first >= 360 || bearing_second < 0 ||
                        bearing_second >= 360)
                    {
                        Nan::ThrowError("Bearing values need to be in range 0..360");
                        return false;
                    }

                    params->bearings.push_back(osrm::Bearing{bearing_first, bearing_second});
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

    return true;
}

template <typename ParamType>
bool parseCommonParameters(const v8::Local<v8::Object> obj, ParamType &params)
{
    if (obj->Has(Nan::New("steps").ToLocalChecked()))
    {
        params->steps = obj->Get(Nan::New("steps").ToLocalChecked())->BooleanValue();
    }

    if (obj->Has(Nan::New("geometries").ToLocalChecked()))
    {
        v8::Local<v8::Value> geometries = obj->Get(Nan::New("geometries").ToLocalChecked());

        if (!geometries->IsString())
        {
            Nan::ThrowError("Geometries must be a string: [polyline, geojson]");
            return false;
        }

        std::string geometries_str = *v8::String::Utf8Value(geometries);

        if (geometries_str == "polyline")
        {
            params->geometries = osrm::RouteParameters::GeometriesType::Polyline;
        }
        else if (geometries_str == "geojson")
        {
            params->geometries = osrm::RouteParameters::GeometriesType::GeoJSON;
        }
        else
        {
            Nan::ThrowError("'geometries' param must be one of [polyline, geojson]");
            return false;
        }
    }

    if (obj->Has(Nan::New("overview").ToLocalChecked()))
    {
        v8::Local<v8::Value> overview = obj->Get(Nan::New("overview").ToLocalChecked());

        if (!overview->IsString())
        {
            Nan::ThrowError("Overview must be a string: [simplified, full, false]");
            return false;
        }

        std::string overview_str = *v8::String::Utf8Value(overview);

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

route_parameters_ptr argumentsToRouteParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                                               bool requires_multiple_coordinates)
{
    route_parameters_ptr params = make_unique<osrm::RouteParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return route_parameters_ptr();

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();

    if (obj->Has(Nan::New("continue_straight").ToLocalChecked()))
    {
        auto value = obj->Get(Nan::New("continue_straight").ToLocalChecked());
        if (!value->IsBoolean() && !value->IsNull())
        {
            Nan::ThrowError("'continue_straight' parama must be boolean or null");
        }
        if (value->IsBoolean())
        {
            params->continue_straight = value->BooleanValue();
        }
    }

    if (obj->Has(Nan::New("alternatives").ToLocalChecked()))
    {
        auto value = obj->Get(Nan::New("alternatives").ToLocalChecked());
        if (!value->IsBoolean())
        {
            Nan::ThrowError("'alternatives' parama must be boolean");
        }
        params->alternatives = value->BooleanValue();
    }

    bool parsedSuccessfully = parseCommonParameters(obj, params);
    if (!parsedSuccessfully)
    {
        return route_parameters_ptr();
    }

    return params;
}

tile_parameters_ptr argumentsToTileParameters(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    tile_parameters_ptr params = make_unique<osrm::TileParameters>();

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

    return params;
}

nearest_parameters_ptr argumentsToNearestParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                                                   bool requires_multiple_coordinates)
{
    nearest_parameters_ptr params = make_unique<osrm::NearestParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return nearest_parameters_ptr();

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();

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

table_parameters_ptr argumentsToTableParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                                               bool requires_multiple_coordinates)
{
    table_parameters_ptr params = make_unique<osrm::TableParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return table_parameters_ptr();

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();

    if (obj->Has(Nan::New("sources").ToLocalChecked()))
    {
        v8::Local<v8::Value> sources = obj->Get(Nan::New("sources").ToLocalChecked());

        if (!sources->IsArray())
        {
            Nan::ThrowError("Sources must be an array of indices (or undefined)");
            return table_parameters_ptr();
        }

        v8::Local<v8::Array> sources_array = v8::Local<v8::Array>::Cast(sources);
        for (uint32_t i = 0; i < sources_array->Length(); ++i)
        {
            v8::Local<v8::Value> source = sources_array->Get(i);
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

        if (!destinations->IsArray())
        {
            Nan::ThrowError("Destinations must be an array of indices (or undefined)");
            return table_parameters_ptr();
        }

        v8::Local<v8::Array> destinations_array = v8::Local<v8::Array>::Cast(destinations);
        for (uint32_t i = 0; i < destinations_array->Length(); ++i)
        {
            v8::Local<v8::Value> destination = destinations_array->Get(i);
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

    return params;
}

trip_parameters_ptr argumentsToTripParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                                             bool requires_multiple_coordinates)
{
    trip_parameters_ptr params = make_unique<osrm::TripParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return trip_parameters_ptr();

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();

    bool parsedSuccessfully = parseCommonParameters(obj, params);
    if (!parsedSuccessfully)
    {
        return trip_parameters_ptr();
    }

    return params;
}

match_parameters_ptr argumentsToMatchParameter(const Nan::FunctionCallbackInfo<v8::Value> &args,
                                               bool requires_multiple_coordinates)
{
    match_parameters_ptr params = make_unique<osrm::MatchParameters>();
    bool has_base_params = argumentsToParameter(args, params, requires_multiple_coordinates);
    if (!has_base_params)
        return match_parameters_ptr();

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(args[0]).ToLocalChecked();

    if (obj->Has(Nan::New("timestamps").ToLocalChecked()))
    {
        v8::Local<v8::Value> timestamps = obj->Get(Nan::New("timestamps").ToLocalChecked());

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
            if (!timestamp->IsNumber())
            {
                Nan::ThrowError("Timestamps array items must be numbers");
                return match_parameters_ptr();
            }
            params->timestamps.emplace_back(static_cast<unsigned>(timestamp->NumberValue()));
        }
    }

    bool parsedSuccessfully = parseCommonParameters(obj, params);
    if (!parsedSuccessfully)
    {
        return match_parameters_ptr();
    }

    return params;
}

struct RouteQueryBaton;
struct NearestQueryBaton;
struct TableQueryBaton;
struct TripQueryBaton;
struct TileQueryBaton;
struct MatchQueryBaton;

class Engine final : public Nan::ObjectWrap
{
  public:
    static void Initialize(v8::Handle<v8::Object>);
    static void New(const Nan::FunctionCallbackInfo<v8::Value> &args);

    static void route(const Nan::FunctionCallbackInfo<v8::Value> &args);
    static void nearest(const Nan::FunctionCallbackInfo<v8::Value> &args);
    static void table(const Nan::FunctionCallbackInfo<v8::Value> &args);
    static void tile(const Nan::FunctionCallbackInfo<v8::Value> &args);
    static void match(const Nan::FunctionCallbackInfo<v8::Value> &args);
    static void trip(const Nan::FunctionCallbackInfo<v8::Value> &args);

    static void ParseResult(const osrm::engine::Status result_status_code,
                            osrm::json::Object& result);

    template <typename BatonType, typename ParamType, typename AsyncType, typename AfterType>
    static void Run(const Nan::FunctionCallbackInfo<v8::Value> &args,
                    ParamType params,
                    AsyncType async_fn,
                    AfterType after_fn);

    static void AsyncRunRoute(uv_work_t *);
    static void AsyncRunNearest(uv_work_t *);
    static void AsyncRunTable(uv_work_t *);
    static void AsyncRunMatch(uv_work_t *);
    static void AsyncRunTrip(uv_work_t *);
    static void AsyncRunTile(uv_work_t *);

    template <typename BatonType> static void AfterRun(BatonType *closure);

    static void AfterRunRoute(uv_work_t *);
    static void AfterRunNearest(uv_work_t *);
    static void AfterRunTable(uv_work_t *);
    static void AfterRunMatch(uv_work_t *);
    static void AfterRunTrip(uv_work_t *);
    static void AfterRunTile(uv_work_t *);

  private:
    Engine(osrm::EngineConfig &engine_config)
        : Nan::ObjectWrap(), this_(make_unique<osrm::OSRM>(engine_config))
    {
    }

    static Nan::Persistent<v8::Function> constructor;
    osrm_ptr this_;
};

Nan::Persistent<v8::Function> Engine::constructor;

void Engine::Initialize(v8::Handle<v8::Object> target)
{
    v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(Engine::New);

    function_template->InstanceTemplate()->SetInternalFieldCount(1);
    function_template->SetClassName(Nan::New("OSRM").ToLocalChecked());

    SetPrototypeMethod(function_template, "route", route);
    SetPrototypeMethod(function_template, "nearest", nearest);
    SetPrototypeMethod(function_template, "table", table);
    SetPrototypeMethod(function_template, "tile", tile);
    SetPrototypeMethod(function_template, "match", match);
    SetPrototypeMethod(function_template, "trip", trip);

    constructor.Reset(function_template->GetFunction());
    Nan::Set(target, Nan::New("OSRM").ToLocalChecked(), function_template->GetFunction());
}

void Engine::New(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    Nan::HandleScope scope;
    if (!args.IsConstructCall())
    {
        Nan::ThrowTypeError("Cannot call constructor as function, you need to use 'new' "
                            "keyword");
        return;
    }

    try
    {
        auto engine_config_ptr = argumentsToEngineConfig(args);
        if (!engine_config_ptr)
        {
            return;
        }
        auto engine_ptr = new Engine(*engine_config_ptr);
        engine_ptr->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
    }
    catch (std::exception const &ex)
    {
        Nan::ThrowTypeError(ex.what());
    }
}

template <typename ResultT> struct RunQueryBaton
{
    uv_work_t request;
    Engine *machine;
    ResultT result;
    Nan::Persistent<v8::Function> cb;
    std::string error;
};

struct RouteQueryBaton : public RunQueryBaton<osrm::json::Object>
{
    route_parameters_ptr params;
};

struct TableQueryBaton : public RunQueryBaton<osrm::json::Object>
{
    table_parameters_ptr params;
};

struct NearestQueryBaton : public RunQueryBaton<osrm::json::Object>
{
    nearest_parameters_ptr params;
};

struct TripQueryBaton : public RunQueryBaton<osrm::json::Object>
{
    trip_parameters_ptr params;
};

struct TileQueryBaton : public RunQueryBaton<std::string>
{
    tile_parameters_ptr params;
};

struct MatchQueryBaton : public RunQueryBaton<osrm::json::Object>
{
    match_parameters_ptr params;
};

void Engine::route(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    Nan::HandleScope scope;
    route_parameters_ptr params = argumentsToRouteParameter(args, true);
    if (!params)
        return;

    BOOST_ASSERT(params->IsValid());

    Run<RouteQueryBaton>(args, std::move(params), AsyncRunRoute, AfterRunRoute);
}

void Engine::match(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    Nan::HandleScope scope;
    match_parameters_ptr params = argumentsToMatchParameter(args, true);
    if (!params)
        return;

    BOOST_ASSERT(params->IsValid());

    Run<MatchQueryBaton>(args, std::move(params), AsyncRunMatch, AfterRunMatch);
}

void Engine::trip(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    Nan::HandleScope scope;
    trip_parameters_ptr params = argumentsToTripParameter(args, true);
    if (!params)
        return;

    BOOST_ASSERT(params->IsValid());

    Run<TripQueryBaton>(args, std::move(params), AsyncRunTrip, AfterRunTrip);
}

void Engine::table(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    Nan::HandleScope scope;
    table_parameters_ptr params = argumentsToTableParameter(args, true);
    if (!params)
        return;

    BOOST_ASSERT(params->IsValid());

    Run<TableQueryBaton>(args, std::move(params), AsyncRunTable, AfterRunTable);
}

void Engine::nearest(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    Nan::HandleScope scope;
    nearest_parameters_ptr params = argumentsToNearestParameter(args, false);
    if (!params)
        return;

    BOOST_ASSERT(params->IsValid());

    Run<NearestQueryBaton>(args, std::move(params), AsyncRunNearest, AfterRunNearest);
}

void Engine::tile(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    Nan::HandleScope scope;
    tile_parameters_ptr params = argumentsToTileParameters(args);
    if (!params)
        return;

    BOOST_ASSERT(params->IsValid());

    Run<TileQueryBaton>(args, std::move(params), AsyncRunTile, AfterRunTile);
}

template <typename BatonType, typename ParamType, typename AsyncType, typename AfterType>
void Engine::Run(const Nan::FunctionCallbackInfo<v8::Value> &args,
                 ParamType params,
                 AsyncType asnyc_fn,
                 AfterType after_fn)
{
    Nan::HandleScope scope;
    v8::Local<v8::Value> callback = args[args.Length() - 1];

    if (!callback->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    auto closure = new BatonType();
    closure->request.data = closure;
    closure->machine = ObjectWrap::Unwrap<Engine>(args.This());
    closure->params = std::move(params);
    closure->error = "";
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, asnyc_fn,
                  reinterpret_cast<uv_after_work_cb>(after_fn));
    closure->machine->Ref();
    return;
}

void Engine::ParseResult(const osrm::Status result_status, osrm::json::Object& result)
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

void Engine::AsyncRunRoute(uv_work_t *req)
{
    RouteQueryBaton *closure = static_cast<RouteQueryBaton *>(req->data);
    try
    {
        const auto status = closure->machine->this_->Route(*closure->params, closure->result);

        ParseResult(status, closure->result);
    }
    catch (std::exception const &ex)
    {
        closure->error = ex.what();
    }
}

void Engine::AsyncRunNearest(uv_work_t *req)
{
    NearestQueryBaton *closure = static_cast<NearestQueryBaton *>(req->data);
    try
    {
        const auto status = closure->machine->this_->Nearest(*closure->params, closure->result);

        ParseResult(status, closure->result);
    }
    catch (std::exception const &ex)
    {
        closure->error = ex.what();
    }
}

void Engine::AsyncRunTable(uv_work_t *req)
{
    TableQueryBaton *closure = static_cast<TableQueryBaton *>(req->data);
    try
    {
        const auto status = closure->machine->this_->Table(*closure->params, closure->result);

        ParseResult(status, closure->result);
    }
    catch (std::exception const &ex)
    {
        closure->error = ex.what();
    }
}

void Engine::AsyncRunTrip(uv_work_t *req)
{
    TripQueryBaton *closure = static_cast<TripQueryBaton *>(req->data);
    try
    {
        const auto status = closure->machine->this_->Trip(*closure->params, closure->result);

        ParseResult(status, closure->result);
    }
    catch (std::exception const &ex)
    {
        closure->error = ex.what();
    }
}

void Engine::AsyncRunTile(uv_work_t *req)
{
    TileQueryBaton *closure = static_cast<TileQueryBaton *>(req->data);
    try
    {
        const auto status = closure->machine->this_->Tile(*closure->params, closure->result);
        if (status == osrm::Status::Error)
        {
            throw std::logic_error("InvalidRequest");
        }
    }
    catch (std::exception const &ex)
    {
        closure->error = ex.what();
    }
}

void Engine::AsyncRunMatch(uv_work_t *req)
{
    MatchQueryBaton *closure = static_cast<MatchQueryBaton *>(req->data);
    try
    {
        const auto status = closure->machine->this_->Match(*closure->params, closure->result);

        ParseResult(status, closure->result);
    }
    catch (std::exception const &ex)
    {
        closure->error = ex.what();
    }
}

template <typename ResultT> v8::Local<v8::Value> render(const ResultT &result);

template <> v8::Local<v8::Value> render(const std::string &result)
{
    return Nan::CopyBuffer(result.data(), result.size()).ToLocalChecked();
}

template <> v8::Local<v8::Value> render(const osrm::json::Object &result)
{
    v8::Local<v8::Value> value;
    renderToV8(value, result);
    return value;
}

template <typename BatonType> void Engine::AfterRun(BatonType *closure)
{
    Nan::TryCatch try_catch;
    if (closure->error.size() > 0)
    {
        v8::Local<v8::Value> argv[1] = {Nan::Error(closure->error.c_str())};
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> result = render(closure->result);
        v8::Local<v8::Value> argv[2] = {Nan::Null(), result};
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    if (try_catch.HasCaught())
    {
        Nan::FatalException(try_catch);
    }
    closure->machine->Unref();
    closure->cb.Reset();
    delete closure;
}

void Engine::AfterRunRoute(uv_work_t *req)
{
    Nan::HandleScope scope;
    RouteQueryBaton *closure = static_cast<RouteQueryBaton *>(req->data);
    AfterRun(closure);
}

void Engine::AfterRunTable(uv_work_t *req)
{
    Nan::HandleScope scope;
    TableQueryBaton *closure = static_cast<TableQueryBaton *>(req->data);
    AfterRun(closure);
}

void Engine::AfterRunNearest(uv_work_t *req)
{
    Nan::HandleScope scope;
    NearestQueryBaton *closure = static_cast<NearestQueryBaton *>(req->data);
    AfterRun(closure);
}

void Engine::AfterRunTrip(uv_work_t *req)
{
    Nan::HandleScope scope;
    TripQueryBaton *closure = static_cast<TripQueryBaton *>(req->data);
    AfterRun(closure);
}

void Engine::AfterRunTile(uv_work_t *req)
{
    Nan::HandleScope scope;
    TileQueryBaton *closure = static_cast<TileQueryBaton *>(req->data);
    AfterRun(closure);
}

void Engine::AfterRunMatch(uv_work_t *req)
{
    Nan::HandleScope scope;
    MatchQueryBaton *closure = static_cast<MatchQueryBaton *>(req->data);
    AfterRun(closure);
}

extern "C" {
static void start(v8::Handle<v8::Object> target) { Engine::Initialize(target); }
}

} // namespace node_osrm

NODE_MODULE(osrm, node_osrm::start)
