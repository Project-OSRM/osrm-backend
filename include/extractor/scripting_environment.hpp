#ifndef SCRIPTING_ENVIRONMENT_HPP
#define SCRIPTING_ENVIRONMENT_HPP

#include "extractor/internal_extractor_edge.hpp"
#include "extractor/profile_properties.hpp"

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

struct ExtractionNode;
struct ExtractionWay;

struct ScriptingContext {
    ScriptingContext() = default;
    virtual ~ScriptingContext() = default;
    virtual std::unordered_set<std::string> getNameSuffixList() = 0;
    virtual std::vector<std::string> getExceptions() = 0;
    virtual void setupSources() = 0;
    virtual int getTurnPenalty(double /* angle */) { return 0; }
    virtual void processNode(const osmium::Node &, ExtractionNode &result) = 0;
    virtual void processWay(const osmium::Way &, ExtractionWay &result) = 0;
    virtual void processSegment(const osrm::util::Coordinate &source,
                                const osrm::util::Coordinate &target,
                                double distance,
                                InternalExtractorEdge::WeightData &weight) = 0;

    ProfileProperties properties;
};

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

    virtual ScriptingContext &GetContext() = 0;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_HPP */
