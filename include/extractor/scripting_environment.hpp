#ifndef SCRIPTING_ENVIRONMENT_HPP
#define SCRIPTING_ENVIRONMENT_HPP

#include "extractor/internal_extractor_edge.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/restriction.hpp"

#include <osmium/memory/buffer.hpp>

#include <boost/optional/optional.hpp>

#include <tbb/concurrent_vector.h>

#include <string>
#include <unordered_set>
#include <vector>

namespace osmium
{
class Node;
class Way;
}

namespace osrm
{

namespace util
{
struct Coordinate;
}

namespace extractor
{

struct RestrictionParser;
struct ExtractionNode;
struct ExtractionWay;

/**
 * Creates a lua context and binds osmium way, node and relation objects and
 * ExtractionWay and ExtractionNode to lua objects.
 *
 * Each thread has its own lua state which is implemented with thread specific
 * storage from TBB.
 */
class ScriptingEnvironment
{
  public:
    ScriptingEnvironment() = default;
    ScriptingEnvironment(const ScriptingEnvironment &) = delete;
    ScriptingEnvironment &operator=(const ScriptingEnvironment &) = delete;
    virtual ~ScriptingEnvironment() = default;

    virtual const ProfileProperties &GetProfileProperties() = 0;

    virtual std::unordered_set<std::string> GetNameSuffixList() = 0;
    virtual std::vector<std::string> GetExceptions() = 0;
    virtual void SetupSources() = 0;
    virtual void ProcessTurnPenalties(std::vector<float> &angles) = 0;
    virtual void ProcessSegment(const osrm::util::Coordinate &source,
                                const osrm::util::Coordinate &target,
                                double distance,
                                InternalExtractorEdge::WeightData &weight) = 0;
    virtual void
    ProcessElements(const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
                    const RestrictionParser &restriction_parser,
                    tbb::concurrent_vector<std::pair<std::size_t, ExtractionNode>> &resulting_nodes,
                    tbb::concurrent_vector<std::pair<std::size_t, ExtractionWay>> &resulting_ways,
                    tbb::concurrent_vector<boost::optional<InputRestrictionContainer>>
                        &resulting_restrictions) = 0;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_HPP */
