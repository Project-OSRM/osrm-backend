#ifndef ENGINE_API_BASE_RESULT_HPP
#define ENGINE_API_BASE_RESULT_HPP

#include <flatbuffers/flatbuffers.h>
#include <mapbox/variant.hpp>

#include <string>

#include "util/json_container.hpp"

namespace osrm
{
namespace engine
{
namespace api
{
using ResultT =
    mapbox::util::variant<util::json::Object, std::string, flatbuffers::FlatBufferBuilder>;
} // namespace api
} // namespace engine
} // namespace osrm

#endif
