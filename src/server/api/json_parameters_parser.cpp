#include "server/api/json_parameters_parser.hpp"

#include "engine/api/base_parameters.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"

#include "engine/approach.hpp"
#include "engine/bearing.hpp"
#include "engine/polyline_compressor.hpp"

#include "util/coordinate.hpp"

#include <boost/numeric/conversion/cast.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace osrm::server::api
{

namespace
{
namespace rj = rapidjson;

// Small helper: records an error message and signals failure (for functions returning bool).
bool fail(std::string &error, std::string message)
{
    error = std::move(message);
    return false;
}

// Same, for functions returning std::optional<...>.
std::nullopt_t failOpt(std::string &error, std::string message)
{
    error = std::move(message);
    return std::nullopt;
}

// Parses the request body into a DOM, reporting syntax errors through `error`.
bool parseDocument(const std::string &json_body, rj::Document &doc, std::string &error)
{
    if (doc.Parse(json_body.c_str(), json_body.size()).HasParseError())
        return fail(error,
                    std::string{"invalid JSON: "} + rj::GetParseError_En(doc.GetParseError()));
    return true;
}

// --- String -> enum mappings (mirror the symbol tables in base_parameters_grammar.hpp
//     and route_parameters_grammar.hpp) ---

std::optional<engine::api::BaseParameters::SnappingType> parseSnapping(std::string_view value)
{
    using SnappingType = engine::api::BaseParameters::SnappingType;
    if (value == "default")
        return SnappingType::Default;
    if (value == "any")
        return SnappingType::Any;
    return std::nullopt;
}

std::optional<engine::api::BaseParameters::OutputFormatType> parseFormat(std::string_view value)
{
    using OutputFormatType = engine::api::BaseParameters::OutputFormatType;
    if (value == "json")
        return OutputFormatType::JSON;
    if (value == "flatbuffers")
        return OutputFormatType::FLATBUFFERS;
    return std::nullopt;
}

std::optional<engine::Approach> parseApproach(std::string_view value)
{
    if (value == "unrestricted")
        return engine::Approach::UNRESTRICTED;
    if (value == "curb")
        return engine::Approach::CURB;
    if (value == "opposite")
        return engine::Approach::OPPOSITE;
    return std::nullopt;
}

// --- coordinates ---

bool parseCoordinates(const rj::Value &value,
                      std::vector<util::Coordinate> &coordinates,
                      std::string &error)
{
    // Polyline string form: "polyline(...)" or "polyline6(...)"
    if (value.IsString())
    {
        const std::string_view s{value.GetString(), value.GetStringLength()};
        constexpr std::string_view poly6_prefix = "polyline6(";
        constexpr std::string_view poly_prefix = "polyline(";
        if (s.size() >= poly6_prefix.size() + 1 && s.starts_with(poly6_prefix) && s.back() == ')')
        {
            const std::string content{
                s.substr(poly6_prefix.size(), s.size() - poly6_prefix.size() - 1)};
            coordinates = engine::decodePolyline<1000000>(content);
            return true;
        }
        if (s.size() >= poly_prefix.size() + 1 && s.starts_with(poly_prefix) && s.back() == ')')
        {
            const std::string content{
                s.substr(poly_prefix.size(), s.size() - poly_prefix.size() - 1)};
            coordinates = engine::decodePolyline(content);
            return true;
        }
        return fail(error,
                    "coordinates string must be of the form polyline(...) or polyline6(...)");
    }

    if (!value.IsArray())
        return fail(error, "coordinates must be an array of [longitude, latitude] pairs");

    for (const auto &element : value.GetArray())
    {
        if (!element.IsArray() || element.Size() != 2 || !element[0].IsNumber() ||
            !element[1].IsNumber())
            return fail(error,
                        "each coordinate must be an array of two numbers [longitude, latitude]");

        try
        {
            coordinates.emplace_back(
                util::toFixed(util::UnsafeFloatLongitude{element[0].GetDouble()}),
                util::toFixed(util::UnsafeFloatLatitude{element[1].GetDouble()}));
        }
        catch (const boost::numeric::bad_numeric_cast &)
        {
            return fail(error, "coordinate value out of range");
        }
    }
    return true;
}

// --- base parameters (shared by all services) ---

bool parseBaseParameters(const rj::Value &doc,
                         engine::api::BaseParameters &params,
                         std::string &error)
{
    if (!doc.IsObject())
        return fail(error, "request body must be a JSON object");

    // coordinates (required)
    const auto coords_it = doc.FindMember("coordinates");
    if (coords_it == doc.MemberEnd())
        return fail(error, "missing required member: coordinates");
    if (!parseCoordinates(coords_it->value, params.coordinates, error))
        return false;

    // bearings: [ [value, range] | null, ... ]
    if (const auto it = doc.FindMember("bearings"); it != doc.MemberEnd())
    {
        if (!it->value.IsArray())
            return fail(error, "bearings must be an array");
        for (const auto &b : it->value.GetArray())
        {
            if (b.IsNull())
            {
                params.bearings.push_back(std::nullopt);
            }
            else if (b.IsArray() && b.Size() == 2 && b[0].IsInt() && b[1].IsInt())
            {
                params.bearings.push_back(
                    engine::Bearing{static_cast<std::int16_t>(b[0].GetInt()),
                                    static_cast<std::int16_t>(b[1].GetInt())});
            }
            else
            {
                return fail(error, "each bearing must be null or an array of two integers");
            }
        }
    }

    // radiuses: [ number | "unlimited" | null, ... ]
    if (const auto it = doc.FindMember("radiuses"); it != doc.MemberEnd())
    {
        if (!it->value.IsArray())
            return fail(error, "radiuses must be an array");
        for (const auto &r : it->value.GetArray())
        {
            if (r.IsNull())
                params.radiuses.push_back(std::nullopt);
            else if (r.IsNumber())
                params.radiuses.push_back(r.GetDouble());
            else if (r.IsString() && std::string_view{r.GetString()} == "unlimited")
                params.radiuses.push_back(std::numeric_limits<double>::infinity());
            else
                return fail(error, "each radius must be null, a number or \"unlimited\"");
        }
    }

    // hints: [ base64 string | null, ... ]
    // Hints are deprecated: the engine no longer uses them to look up phantom nodes, so the
    // encoded payload is not decoded here. Only the shape is validated, and one (empty) entry
    // per element is recorded so that the hints/coordinates size check in IsValid() sees the
    // same counts as the URL API.
    if (const auto it = doc.FindMember("hints"); it != doc.MemberEnd())
    {
        if (!it->value.IsArray())
            return fail(error, "hints must be an array");
        for (const auto &h : it->value.GetArray())
        {
            if (!h.IsNull() && !h.IsString())
                return fail(error, "each hint must be null or a base64 string");
            params.hints.emplace_back(std::nullopt);
        }
    }

    // approaches: [ "curb" | "opposite" | "unrestricted" | null, ... ]
    if (const auto it = doc.FindMember("approaches"); it != doc.MemberEnd())
    {
        if (!it->value.IsArray())
            return fail(error, "approaches must be an array");
        for (const auto &a : it->value.GetArray())
        {
            if (a.IsNull())
            {
                params.approaches.push_back(std::nullopt);
            }
            else if (a.IsString())
            {
                const auto approach = parseApproach({a.GetString(), a.GetStringLength()});
                if (!approach)
                    return fail(error, "invalid approach value");
                params.approaches.push_back(approach);
            }
            else
            {
                return fail(error, "each approach must be null or a string");
            }
        }
    }

    // exclude: [ string, ... ]
    if (const auto it = doc.FindMember("exclude"); it != doc.MemberEnd())
    {
        if (!it->value.IsArray())
            return fail(error, "exclude must be an array of strings");
        for (const auto &e : it->value.GetArray())
        {
            if (!e.IsString())
                return fail(error, "each exclude entry must be a string");
            params.exclude.emplace_back(e.GetString(), e.GetStringLength());
        }
    }

    // generate_hints: bool
    if (const auto it = doc.FindMember("generate_hints"); it != doc.MemberEnd())
    {
        if (!it->value.IsBool())
            return fail(error, "generate_hints must be a boolean");
        params.generate_hints = it->value.GetBool();
    }

    // skip_waypoints: bool
    if (const auto it = doc.FindMember("skip_waypoints"); it != doc.MemberEnd())
    {
        if (!it->value.IsBool())
            return fail(error, "skip_waypoints must be a boolean");
        params.skip_waypoints = it->value.GetBool();
    }

    // snapping: "default" | "any"
    if (const auto it = doc.FindMember("snapping"); it != doc.MemberEnd())
    {
        if (!it->value.IsString())
            return fail(error, "snapping must be a string");
        const auto snapping = parseSnapping({it->value.GetString(), it->value.GetStringLength()});
        if (!snapping)
            return fail(error, "invalid snapping value");
        params.snapping = *snapping;
    }

    // format: "json" | "flatbuffers"
    if (const auto it = doc.FindMember("format"); it != doc.MemberEnd())
    {
        if (!it->value.IsString())
            return fail(error, "format must be a string");
        const auto format = parseFormat({it->value.GetString(), it->value.GetStringLength()});
        if (!format)
            return fail(error, "invalid format value");
        params.format = format;
    }

    return true;
}

// Parses a JSON array of non-negative integers into a vector<size_t> (used for waypoints,
// sources and destinations).
bool parseIndexArray(const rj::Value &value,
                     const char *name,
                     std::vector<std::size_t> &out,
                     std::string &error)
{
    if (!value.IsArray())
        return fail(error, std::string{name} + " must be an array of indices");
    for (const auto &v : value.GetArray())
    {
        if (!v.IsUint64())
            return fail(error, std::string{name} + " entries must be non-negative integers");
        out.push_back(static_cast<std::size_t>(v.GetUint64()));
    }
    return true;
}

// --- route parameters (shared by /route and /match) ---

// Mirrors how the URL grammar composes route_grammar::route_options into both the route and the
// match root rule: MatchParameters derives from RouteParameters and accepts the same options.
bool parseRouteParameters(const rj::Value &doc,
                          engine::api::RouteParameters &params,
                          std::string &error)
{
    using engine::api::RouteParameters;

    if (!parseBaseParameters(doc, params, error))
        return false;

    if (const auto it = doc.FindMember("steps"); it != doc.MemberEnd())
    {
        if (!it->value.IsBool())
            return fail(error, "steps must be a boolean");
        params.steps = it->value.GetBool();
    }

    if (const auto it = doc.FindMember("alternatives"); it != doc.MemberEnd())
    {
        if (it->value.IsBool())
        {
            params.alternatives = it->value.GetBool();
            params.number_of_alternatives = it->value.GetBool() ? 1u : 0u;
        }
        else if (it->value.IsUint())
        {
            const auto n = it->value.GetUint();
            params.number_of_alternatives = n;
            params.alternatives = n > 0;
        }
        else
        {
            return fail(error, "alternatives must be a boolean or a non-negative integer");
        }
    }

    if (const auto it = doc.FindMember("continue_straight"); it != doc.MemberEnd())
    {
        if (it->value.IsBool())
            params.continue_straight = it->value.GetBool();
        else if (!(it->value.IsString() && std::string_view{it->value.GetString()} == "default"))
            return fail(error, "continue_straight must be a boolean or \"default\"");
        // "default" leaves continue_straight unset (std::nullopt).
    }

    if (const auto it = doc.FindMember("geometries"); it != doc.MemberEnd())
    {
        if (!it->value.IsString())
            return fail(error, "geometries must be a string");
        const std::string_view v{it->value.GetString(), it->value.GetStringLength()};
        if (v == "polyline")
            params.geometries = RouteParameters::GeometriesType::Polyline;
        else if (v == "polyline6")
            params.geometries = RouteParameters::GeometriesType::Polyline6;
        else if (v == "geojson")
            params.geometries = RouteParameters::GeometriesType::GeoJSON;
        else
            return fail(error, "invalid geometries value");
    }

    if (const auto it = doc.FindMember("overview"); it != doc.MemberEnd())
    {
        if (!it->value.IsString())
            return fail(error, "overview must be a string");
        const std::string_view v{it->value.GetString(), it->value.GetStringLength()};
        if (v == "simplified")
            params.overview = RouteParameters::OverviewType::Simplified;
        else if (v == "full")
            params.overview = RouteParameters::OverviewType::Full;
        else if (v == "false")
            params.overview = RouteParameters::OverviewType::False;
        else if (v == "by_legs")
            params.overview = RouteParameters::OverviewType::ByLegs;
        else
            return fail(error, "invalid overview value");
    }

    if (const auto it = doc.FindMember("annotations"); it != doc.MemberEnd())
    {
        using AnnotationsType = RouteParameters::AnnotationsType;
        const auto to_type = [](std::string_view v) -> std::optional<AnnotationsType>
        {
            if (v == "duration")
                return AnnotationsType::Duration;
            if (v == "nodes")
                return AnnotationsType::Nodes;
            if (v == "distance")
                return AnnotationsType::Distance;
            if (v == "weight")
                return AnnotationsType::Weight;
            if (v == "datasources")
                return AnnotationsType::Datasources;
            if (v == "speed")
                return AnnotationsType::Speed;
            return std::nullopt;
        };

        if (it->value.IsBool())
        {
            params.annotations_type =
                it->value.GetBool() ? AnnotationsType::All : AnnotationsType::None;
        }
        else if (it->value.IsArray())
        {
            auto type = AnnotationsType::None;
            for (const auto &a : it->value.GetArray())
            {
                if (!a.IsString())
                    return fail(error, "annotation entries must be strings");
                const auto parsed = to_type({a.GetString(), a.GetStringLength()});
                if (!parsed)
                    return fail(error, "invalid annotation value");
                type = type | *parsed;
            }
            params.annotations_type = type;
        }
        else
        {
            return fail(error, "annotations must be a boolean or an array of strings");
        }
        params.annotations = params.annotations_type != AnnotationsType::None;
    }

    if (const auto it = doc.FindMember("waypoints"); it != doc.MemberEnd())
    {
        if (!parseIndexArray(it->value, "waypoints", params.waypoints, error))
            return false;
    }

    return true;
}

} // namespace

template <>
std::optional<engine::api::RouteParameters> parseJSONParameters(const std::string &json_body,
                                                                std::string &error)
{
    using engine::api::RouteParameters;

    rj::Document doc;
    if (!parseDocument(json_body, doc, error))
        return std::nullopt;

    RouteParameters params;
    if (!parseRouteParameters(doc, params, error))
        return std::nullopt;

    return params;
}

template <>
std::optional<engine::api::MatchParameters> parseJSONParameters(const std::string &json_body,
                                                                std::string &error)
{
    using engine::api::MatchParameters;

    rj::Document doc;
    if (!parseDocument(json_body, doc, error))
        return std::nullopt;

    MatchParameters params;
    if (!parseRouteParameters(doc, params, error))
        return std::nullopt;

    // timestamps: [ non-negative integer, ... ]
    if (const auto it = doc.FindMember("timestamps"); it != doc.MemberEnd())
    {
        if (!it->value.IsArray())
            return failOpt(error, "timestamps must be an array of integers");
        for (const auto &t : it->value.GetArray())
        {
            if (!t.IsUint())
                return failOpt(error, "timestamps entries must be non-negative integers");
            params.timestamps.push_back(t.GetUint());
        }
    }

    // gaps: "split" | "ignore"
    if (const auto it = doc.FindMember("gaps"); it != doc.MemberEnd())
    {
        if (!it->value.IsString())
            return failOpt(error, "gaps must be a string");
        const std::string_view v{it->value.GetString(), it->value.GetStringLength()};
        if (v == "split")
            params.gaps = MatchParameters::GapsType::Split;
        else if (v == "ignore")
            params.gaps = MatchParameters::GapsType::Ignore;
        else
            return failOpt(error, "invalid gaps value");
    }

    // tidy: bool
    if (const auto it = doc.FindMember("tidy"); it != doc.MemberEnd())
    {
        if (!it->value.IsBool())
            return failOpt(error, "tidy must be a boolean");
        params.tidy = it->value.GetBool();
    }

    return params;
}

template <>
std::optional<engine::api::TableParameters> parseJSONParameters(const std::string &json_body,
                                                                std::string &error)
{
    using engine::api::TableParameters;

    rj::Document doc;
    if (!parseDocument(json_body, doc, error))
        return std::nullopt;

    TableParameters params;
    if (!parseBaseParameters(doc, params, error))
        return std::nullopt;

    if (const auto it = doc.FindMember("sources"); it != doc.MemberEnd())
    {
        if (!parseIndexArray(it->value, "sources", params.sources, error))
            return std::nullopt;
    }

    if (const auto it = doc.FindMember("destinations"); it != doc.MemberEnd())
    {
        if (!parseIndexArray(it->value, "destinations", params.destinations, error))
            return std::nullopt;
    }

    if (const auto it = doc.FindMember("annotations"); it != doc.MemberEnd())
    {
        using AnnotationsType = TableParameters::AnnotationsType;
        const auto to_type = [](std::string_view v) -> std::optional<AnnotationsType>
        {
            if (v == "duration")
                return AnnotationsType::Duration;
            if (v == "distance")
                return AnnotationsType::Distance;
            return std::nullopt;
        };

        if (it->value.IsBool())
        {
            params.annotations = it->value.GetBool() ? AnnotationsType::All : AnnotationsType::None;
        }
        else if (it->value.IsArray())
        {
            auto type = AnnotationsType::None;
            for (const auto &a : it->value.GetArray())
            {
                if (!a.IsString())
                    return failOpt(error, "annotation entries must be strings");
                const auto parsed = to_type({a.GetString(), a.GetStringLength()});
                if (!parsed)
                    return failOpt(error, "invalid annotation value");
                type = type | *parsed;
            }
            params.annotations = type;
        }
        else
        {
            return failOpt(error, "annotations must be a boolean or an array of strings");
        }
    }

    if (const auto it = doc.FindMember("fallback_speed"); it != doc.MemberEnd())
    {
        if (!it->value.IsNumber())
            return failOpt(error, "fallback_speed must be a number");
        params.fallback_speed = it->value.GetDouble();
    }

    if (const auto it = doc.FindMember("fallback_coordinate"); it != doc.MemberEnd())
    {
        if (!it->value.IsString())
            return failOpt(error, "fallback_coordinate must be a string");
        const std::string_view v{it->value.GetString(), it->value.GetStringLength()};
        if (v == "input")
            params.fallback_coordinate_type = TableParameters::FallbackCoordinateType::Input;
        else if (v == "snapped")
            params.fallback_coordinate_type = TableParameters::FallbackCoordinateType::Snapped;
        else
            return failOpt(error, "invalid fallback_coordinate value");
    }

    if (const auto it = doc.FindMember("scale_factor"); it != doc.MemberEnd())
    {
        if (!it->value.IsNumber())
            return failOpt(error, "scale_factor must be a number");
        params.scale_factor = it->value.GetDouble();
    }

    return params;
}

} // namespace osrm::server::api
