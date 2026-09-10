#ifndef OSRM_EXTRACTOR_DRIVING_SIDE_INDEX_HPP
#define OSRM_EXTRACTOR_DRIVING_SIDE_INDEX_HPP

#include <tbb/enumerable_thread_specific.h>

#include <osmium/osm/box.hpp>
#include <osmium/osm/way.hpp>

#include <memory>
#include <optional>

namespace gauche
{
class Index;
}

namespace osrm::extractor
{

// Answers which side of the road traffic drives on from a way's own
// coordinates, for the ways that OSM tags and location-dependent data leave
// unanswered.
//
// Backed by gauche, whose index is a wasm instance holding its own linear
// memory, so it is neither cheap to copy nor safe to share: one per thread,
// held the same way ScriptingEnvironmentLua holds its Lua contexts.
class DrivingSideIndex
{
  public:
    enum class Mode
    {
        Off,    // never consulted, driving side comes from tags and the profile
        Auto,   // settle the extract from its bounding box, per-way only if it straddles
        Always, // per-way lookups regardless of what the bounding box says
    };

    static std::optional<Mode> ParseMode(const std::string &name);

    explicit DrivingSideIndex(Mode mode);
    ~DrivingSideIndex();

    DrivingSideIndex(const DrivingSideIndex &) = delete;
    DrivingSideIndex &operator=(const DrivingSideIndex &) = delete;

    bool Enabled() const { return mode != Mode::Off; }

    // Classify the whole extract once, before any way is read. Returns the side
    // when every coordinate in the box shares one, which is the common case for
    // a single-country extract and settles the question without ever looking at
    // a way. Returns nullopt when the box straddles a boundary, when the box is
    // missing or invalid, or in Mode::Always, and then NeedsWayLookups() is
    // true and the extractor has to keep node locations around for the ways.
    std::optional<bool> SettleExtent(const osmium::Box &box);

    bool NeedsWayLookups() const { return needs_way_lookups; }

    // nullopt when the index is off, when the way has no usable coordinates, or
    // when gauche cannot answer. Callers fall back to their own default.
    std::optional<bool> IsLeftHandTraffic(const osmium::Way &way) const;

  private:
    gauche::Index *ThreadIndex() const;

    Mode mode;
    bool needs_way_lookups = false;
    std::optional<bool> settled_side;
    mutable tbb::enumerable_thread_specific<std::unique_ptr<gauche::Index>> indexes;
};

} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_DRIVING_SIDE_INDEX_HPP
