#ifndef EXTRACTOR_CALLBACKS_HPP
#define EXTRACTOR_CALLBACKS_HPP

#include "extractor/guidance/turn_lane_types.hpp"
#include "util/typedefs.hpp"

#include <boost/functional/hash.hpp>
#include <boost/optional/optional_fwd.hpp>

#include <string>
#include <unordered_map>

namespace osmium
{
class Node;
class Way;
}

namespace std
{
template <> struct hash<std::tuple<std::string, std::string, std::string, std::string>>
{
    std::size_t
    operator()(const std::tuple<std::string, std::string, std::string, std::string> &mk) const
        noexcept
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, std::get<0>(mk));
        boost::hash_combine(seed, std::get<1>(mk));
        boost::hash_combine(seed, std::get<2>(mk));
        boost::hash_combine(seed, std::get<3>(mk));
        return seed;
    }
};
}

namespace osrm
{
namespace extractor
{

class ExtractionContainers;
struct InputRestrictionContainer;
struct ExtractionNode;
struct ExtractionWay;
struct ProfileProperties;

/**
 * This class is used by the extractor with the results of the
 * osmium based parsing and the customization through the lua profile.
 *
 * It mediates between the multi-threaded extraction process and the external memory containers.
 * Thus the synchronization is handled inside of the extractor.
 */
class ExtractorCallbacks
{
  private:
    // used to deduplicate street names, refs, destinations, pronunciation: actually maps to name
    // ids
    using MapKey = std::tuple<std::string, std::string, std::string, std::string>;
    using MapVal = unsigned;
    std::unordered_map<MapKey, MapVal> string_map;
    guidance::LaneDescriptionMap lane_description_map;
    ExtractionContainers &external_memory;
    bool fallback_to_duration;

  public:
    explicit ExtractorCallbacks(ExtractionContainers &extraction_containers,
                                const ProfileProperties &properties);

    ExtractorCallbacks(const ExtractorCallbacks &) = delete;
    ExtractorCallbacks &operator=(const ExtractorCallbacks &) = delete;

    // warning: caller needs to take care of synchronization!
    void ProcessNode(const osmium::Node &current_node, const ExtractionNode &result_node);

    // warning: caller needs to take care of synchronization!
    void ProcessRestriction(const boost::optional<InputRestrictionContainer> &restriction);

    // warning: caller needs to take care of synchronization!
    void ProcessWay(const osmium::Way &current_way, const ExtractionWay &result_way);

    // destroys the internal laneDescriptionMap
    guidance::LaneDescriptionMap &&moveOutLaneDescriptionMap();
};
}
}

#endif /* EXTRACTOR_CALLBACKS_HPP */
