// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_WAYPOINT_OSRM_ENGINE_API_FBRESULT_H_
#define FLATBUFFERS_GENERATED_WAYPOINT_OSRM_ENGINE_API_FBRESULT_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 24 &&
              FLATBUFFERS_VERSION_MINOR == 3 &&
              FLATBUFFERS_VERSION_REVISION == 25,
             "Non-compatible flatbuffers version included");

#include "position_generated.h"

namespace osrm {
namespace engine {
namespace api {
namespace fbresult {

struct Uint64Pair;

struct Waypoint;
struct WaypointBuilder;

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(8) Uint64Pair FLATBUFFERS_FINAL_CLASS {
 private:
  uint64_t first_;
  uint64_t second_;

 public:
  Uint64Pair()
      : first_(0),
        second_(0) {
  }
  Uint64Pair(uint64_t _first, uint64_t _second)
      : first_(::flatbuffers::EndianScalar(_first)),
        second_(::flatbuffers::EndianScalar(_second)) {
  }
  uint64_t first() const {
    return ::flatbuffers::EndianScalar(first_);
  }
  uint64_t second() const {
    return ::flatbuffers::EndianScalar(second_);
  }
};
FLATBUFFERS_STRUCT_END(Uint64Pair, 16);

struct Waypoint FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef WaypointBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_HINT = 4,
    VT_DISTANCE = 6,
    VT_NAME = 8,
    VT_LOCATION = 10,
    VT_NODES = 12,
    VT_MATCHINGS_INDEX = 14,
    VT_WAYPOINT_INDEX = 16,
    VT_ALTERNATIVES_COUNT = 18,
    VT_TRIPS_INDEX = 20
  };
  const ::flatbuffers::String *hint() const {
    return GetPointer<const ::flatbuffers::String *>(VT_HINT);
  }
  float distance() const {
    return GetField<float>(VT_DISTANCE, 0.0f);
  }
  const ::flatbuffers::String *name() const {
    return GetPointer<const ::flatbuffers::String *>(VT_NAME);
  }
  const osrm::engine::api::fbresult::Position *location() const {
    return GetStruct<const osrm::engine::api::fbresult::Position *>(VT_LOCATION);
  }
  const osrm::engine::api::fbresult::Uint64Pair *nodes() const {
    return GetStruct<const osrm::engine::api::fbresult::Uint64Pair *>(VT_NODES);
  }
  uint32_t matchings_index() const {
    return GetField<uint32_t>(VT_MATCHINGS_INDEX, 0);
  }
  uint32_t waypoint_index() const {
    return GetField<uint32_t>(VT_WAYPOINT_INDEX, 0);
  }
  uint32_t alternatives_count() const {
    return GetField<uint32_t>(VT_ALTERNATIVES_COUNT, 0);
  }
  uint32_t trips_index() const {
    return GetField<uint32_t>(VT_TRIPS_INDEX, 0);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_HINT) &&
           verifier.VerifyString(hint()) &&
           VerifyField<float>(verifier, VT_DISTANCE, 4) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<osrm::engine::api::fbresult::Position>(verifier, VT_LOCATION, 4) &&
           VerifyField<osrm::engine::api::fbresult::Uint64Pair>(verifier, VT_NODES, 8) &&
           VerifyField<uint32_t>(verifier, VT_MATCHINGS_INDEX, 4) &&
           VerifyField<uint32_t>(verifier, VT_WAYPOINT_INDEX, 4) &&
           VerifyField<uint32_t>(verifier, VT_ALTERNATIVES_COUNT, 4) &&
           VerifyField<uint32_t>(verifier, VT_TRIPS_INDEX, 4) &&
           verifier.EndTable();
  }
};

struct WaypointBuilder {
  typedef Waypoint Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_hint(::flatbuffers::Offset<::flatbuffers::String> hint) {
    fbb_.AddOffset(Waypoint::VT_HINT, hint);
  }
  void add_distance(float distance) {
    fbb_.AddElement<float>(Waypoint::VT_DISTANCE, distance, 0.0f);
  }
  void add_name(::flatbuffers::Offset<::flatbuffers::String> name) {
    fbb_.AddOffset(Waypoint::VT_NAME, name);
  }
  void add_location(const osrm::engine::api::fbresult::Position *location) {
    fbb_.AddStruct(Waypoint::VT_LOCATION, location);
  }
  void add_nodes(const osrm::engine::api::fbresult::Uint64Pair *nodes) {
    fbb_.AddStruct(Waypoint::VT_NODES, nodes);
  }
  void add_matchings_index(uint32_t matchings_index) {
    fbb_.AddElement<uint32_t>(Waypoint::VT_MATCHINGS_INDEX, matchings_index, 0);
  }
  void add_waypoint_index(uint32_t waypoint_index) {
    fbb_.AddElement<uint32_t>(Waypoint::VT_WAYPOINT_INDEX, waypoint_index, 0);
  }
  void add_alternatives_count(uint32_t alternatives_count) {
    fbb_.AddElement<uint32_t>(Waypoint::VT_ALTERNATIVES_COUNT, alternatives_count, 0);
  }
  void add_trips_index(uint32_t trips_index) {
    fbb_.AddElement<uint32_t>(Waypoint::VT_TRIPS_INDEX, trips_index, 0);
  }
  explicit WaypointBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<Waypoint> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<Waypoint>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<Waypoint> CreateWaypoint(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<::flatbuffers::String> hint = 0,
    float distance = 0.0f,
    ::flatbuffers::Offset<::flatbuffers::String> name = 0,
    const osrm::engine::api::fbresult::Position *location = nullptr,
    const osrm::engine::api::fbresult::Uint64Pair *nodes = nullptr,
    uint32_t matchings_index = 0,
    uint32_t waypoint_index = 0,
    uint32_t alternatives_count = 0,
    uint32_t trips_index = 0) {
  WaypointBuilder builder_(_fbb);
  builder_.add_trips_index(trips_index);
  builder_.add_alternatives_count(alternatives_count);
  builder_.add_waypoint_index(waypoint_index);
  builder_.add_matchings_index(matchings_index);
  builder_.add_nodes(nodes);
  builder_.add_location(location);
  builder_.add_name(name);
  builder_.add_distance(distance);
  builder_.add_hint(hint);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<Waypoint> CreateWaypointDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    const char *hint = nullptr,
    float distance = 0.0f,
    const char *name = nullptr,
    const osrm::engine::api::fbresult::Position *location = nullptr,
    const osrm::engine::api::fbresult::Uint64Pair *nodes = nullptr,
    uint32_t matchings_index = 0,
    uint32_t waypoint_index = 0,
    uint32_t alternatives_count = 0,
    uint32_t trips_index = 0) {
  auto hint__ = hint ? _fbb.CreateString(hint) : 0;
  auto name__ = name ? _fbb.CreateString(name) : 0;
  return osrm::engine::api::fbresult::CreateWaypoint(
      _fbb,
      hint__,
      distance,
      name__,
      location,
      nodes,
      matchings_index,
      waypoint_index,
      alternatives_count,
      trips_index);
}

}  // namespace fbresult
}  // namespace api
}  // namespace engine
}  // namespace osrm

#endif  // FLATBUFFERS_GENERATED_WAYPOINT_OSRM_ENGINE_API_FBRESULT_H_
