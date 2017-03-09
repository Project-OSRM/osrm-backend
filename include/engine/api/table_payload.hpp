#ifndef ENGINE_TABLE_PAYLOAD_HPP
#define ENGINE_TABLE_PAYLOAD_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/json_factory.hpp"
#include "engine/api/table_parameters.hpp"

#include "engine/datafacade/datafacade_base.hpp"

#include "engine/internal_route_result.hpp"

#include "util/integer_range.hpp"

#include <iterator>

namespace osrm
{
namespace engine
{
namespace api
{

template <typename PayloadType>
using PAYLOAD_ENCODER = util::json::Value (*)(const PayloadType &entry);

template <typename PayloadType>
inline util::json::Value default_duration_encoder(const PayloadType &entry)
{
    if (entry.duration == MAXIMAL_EDGE_DURATION)
        return util::json::Value(util::json::Null());
    return util::json::Value(util::json::Number(entry.duration / 10.));
}

template <typename PayloadType>
inline util::json::Value default_distance_encoder(const PayloadType &entry)
{
    if (entry.distance == MAXIMAL_EDGE_DISTANCE)
        return util::json::Value(util::json::Null());
    return util::json::Value(util::json::Number(entry.distance));
}

template <typename PayloadType> inline util::json::Value empty_encoder(const PayloadType &entry)
{
    return util::json::Value(util::json::Null());
}

template <typename PayloadType>
inline PAYLOAD_ENCODER<PayloadType> get_encoder(TableOutputField component)
{
    throw util::exception("Internal Error: Encoders missing for used payload type.");
}

template <>
inline PAYLOAD_ENCODER<DurationDistancePayload>
get_encoder<DurationDistancePayload>(TableOutputField component)
{
    switch (component)
    {
    case DURATION:
        return default_duration_encoder<DurationDistancePayload>;
    case DISTANCE:
        return default_distance_encoder<DurationDistancePayload>;
    default:
        throw util::exception("Internal Error: Encountered unknown output component type: " +
                              component);
    }
}

template <>
inline PAYLOAD_ENCODER<DurationPayload> get_encoder<DurationPayload>(TableOutputField component)
{
    switch (component)
    {
    case DURATION:
        return default_duration_encoder<DurationPayload>;
    case DISTANCE:
        throw util::exception("Internal Error: Requested distance, but osrm is not configured to "
                              "provide distances."); // TODO: Graceful error handling
    default:
        throw util::exception("Internal Error: Encountered unknown output component type: " +
                              component);
    }
}

} // ns api
} // ns engine
} // ns osrm

#endif
