#ifndef EXTRACTOR_CALLBACKS_HPP
#define EXTRACTOR_CALLBACKS_HPP

#include "extractor/class_data.hpp"
#include "extractor/turn_lane_types.hpp"
#include "util/typedefs.hpp"

#include <boost/functional/hash.hpp>
#include <boost/optional/optional_fwd.hpp>

#include <string>
#include <unordered_map>

namespace osmium
{
class Node;
class Way;
class Relation;
}

namespace std
{
template <> struct hash<std::tuple<std::string, std::string, std::string, std::string, std::string>>
{
    std::size_t operator()(
        const std::tuple<std::string, std::string, std::string, std::string, std::string> &mk) const
        noexcept
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, std::get<0>(mk));
        boost::hash_combine(seed, std::get<1>(mk));
        boost::hash_combine(seed, std::get<2>(mk));
        boost::hash_combine(seed, std::get<3>(mk));
        boost::hash_combine(seed, std::get<4>(mk));
        return seed;
    }
};
}

namespace osrm
{
namespace extractor
{

class ExtractionContainers;
struct ExtractionNode;
struct ExtractionWay;
struct ExtractionRelation;
struct ProfileProperties;
struct InputConditionalTurnRestriction;
struct InputManeuverOverride;

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
    // used to deduplicate street names, refs, destinations, pronunciation, exits:
    // actually maps to name ids
    using MapKey = std::tuple<std::string, std::string, std::string, std::string, std::string>;
    using MapVal = unsigned;
    using StringMap = std::unordered_map<MapKey, MapVal>;
    StringMap string_map;
    ExtractionContainers &external_memory;
    std::unordered_map<std::string, ClassData> &classes_map;
    LaneDescriptionMap &lane_description_map;
    bool fallback_to_duration;
    bool force_split_edges;

  public:
    using ClassesMap = std::unordered_map<std::string, ClassData>;

    explicit ExtractorCallbacks(ExtractionContainers &extraction_containers,
                                std::unordered_map<std::string, ClassData> &classes_map,
                                LaneDescriptionMap &lane_description_map,
                                const ProfileProperties &properties);

    ExtractorCallbacks(const ExtractorCallbacks &) = delete;
    ExtractorCallbacks &operator=(const ExtractorCallbacks &) = delete;

    // warning: caller needs to take care of synchronization!
    void ProcessNode(const osmium::Node &current_node, const ExtractionNode &result_node);

    // warning: caller needs to take care of synchronization!
    void ProcessRestriction(const InputConditionalTurnRestriction &restriction);

    // warning: caller needs to take care of synchronization!
    void ProcessWay(const osmium::Way &current_way, const ExtractionWay &result_way);

    // warning: caller needs to take care of synchronization!
    void ProcessManeuverOverride(const InputManeuverOverride & override);
};
}
}

#endif /* EXTRACTOR_CALLBACKS_HPP */
