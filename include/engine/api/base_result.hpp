#ifndef ENGINE_API_BASE_RESULT_HPP
#define ENGINE_API_BASE_RESULT_HPP

#include <flatbuffers/flatbuffers.h>
#include <variant>

#include <string>

#include "util/json_container.hpp"

namespace osrm::engine::api
{
using ResultT = std::variant<util::json::Object, std::string, flatbuffers::FlatBufferBuilder>;
} // namespace osrm::engine::api

#endif
