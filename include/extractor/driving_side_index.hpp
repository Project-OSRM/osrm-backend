#ifndef OSRM_EXTRACTOR_DRIVING_SIDE_INDEX_HPP
#define OSRM_EXTRACTOR_DRIVING_SIDE_INDEX_HPP

#include <tbb/enumerable_thread_specific.h>

#include <osmium/memory/buffer.hpp>
#include <osmium/osm/box.hpp>
#include <osmium/osm/way.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

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
        Auto,   // settle the whole extract if it lies on one side, else per way
        Always, // classify every way from its own nodes
    };

    static std::optional<Mode> ParseMode(const std::string &name);

    explicit DrivingSideIndex(Mode mode);
    ~DrivingSideIndex();

    DrivingSideIndex(const DrivingSideIndex &) = delete;
    DrivingSideIndex &operator=(const DrivingSideIndex &) = delete;

    bool Enabled() const { return mode != Mode::Off; }

    // Auto has to know the extract's extent before it classifies the first way,
    // and the extent the file declares in its header is not evidence: a hand cut
    // extract can carry a box that does not contain its own data, and settling
    // on a box smaller than the ways in it would put ways on the wrong side.
    // So the extent is measured from the nodes themselves.
    bool ObservesNodes() const { return mode == Mode::Auto; }
    void ObserveNodes(const osmium::memory::Buffer &buffer);

    // Both remaining modes may end up classifying ways one at a time, so both
    // need the way's nodes to carry their locations.
    bool NeedsWayLookups() const { return mode != Mode::Off; }

    // nullopt when the index is off, when the way has no usable coordinates, or
    // when gauche cannot answer. Callers fall back to their own default.
    std::optional<bool> IsLeftHandTraffic(const osmium::Way &way) const;

  private:
    gauche::Index *ThreadIndex() const;

    // Classifies the observed extent, once, on the first way that asks. An OSM
    // file lists all its nodes before its first way and the observation happens
    // in a pipeline stage every way has yet to reach, so the extent is complete
    // by the time this runs.
    void SettleObservedExtent() const;

    Mode mode;

    mutable std::mutex extent_mutex;
    osmium::Box observed_extent;
    mutable std::atomic<bool> settled{false};
    mutable std::optional<bool> settled_side;

    // shared_ptr, not unique_ptr: the deleter is captured where gauche::Index is
    // complete, in the .cpp, so this header can keep it an opaque forward
    // declaration. MSVC instantiates unique_ptr's deleter in every translation
    // unit that destroys one and rejects the incomplete type there.
    mutable tbb::enumerable_thread_specific<std::shared_ptr<gauche::Index>> indexes;
};

} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_DRIVING_SIDE_INDEX_HPP
