#ifndef EXTRACTOR_CALLBACKS_HPP
#define EXTRACTOR_CALLBACKS_HPP

#include "util/typedefs.hpp"
#include <boost/optional/optional_fwd.hpp>
#include <boost/functional/hash.hpp>

#include <string>
#include <unordered_map>

namespace osmium
{
class Node;
class Way;
}

namespace osrm
{
namespace extractor
{

class ExtractionContainers;
struct InputRestrictionContainer;
struct ExtractionNode;
struct ExtractionWay;

/**
 * This class is uses by the extractor with the results of the
 * osmium based parsing and the customization through the lua profile.
 *
 * It mediates between the multi-threaded extraction process and the external memory containers.
 * Thus the synchronization is handled inside of the extractor.
 */
class ExtractorCallbacks
{
  private:
    // used to deduplicate street names and street destinations: actually maps to name ids
    using MapKey = std::pair<std::string, std::string>;
    using MapVal = unsigned;
    std::unordered_map<MapKey, MapVal, boost::hash<MapKey>> string_map;
    ExtractionContainers &external_memory;

  public:
    explicit ExtractorCallbacks(ExtractionContainers &extraction_containers);

    ExtractorCallbacks(const ExtractorCallbacks &) = delete;
    ExtractorCallbacks &operator=(const ExtractorCallbacks &) = delete;

    // warning: caller needs to take care of synchronization!
    void ProcessNode(const osmium::Node &current_node, const ExtractionNode &result_node);

    // warning: caller needs to take care of synchronization!
    void ProcessRestriction(const boost::optional<InputRestrictionContainer> &restriction);

    // warning: caller needs to take care of synchronization!
    void ProcessWay(const osmium::Way &current_way, const ExtractionWay &result_way);
};
}
}

#endif /* EXTRACTOR_CALLBACKS_HPP */
