#ifndef SCRIPTING_ENVIRONMENT_HPP
#define SCRIPTING_ENVIRONMENT_HPP

#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/restriction.hpp"

#include <osmium/memory/buffer.hpp>

#include <boost/optional/optional.hpp>

#include <tbb/concurrent_vector.h>

#include <string>
#include <vector>

namespace osmium
{
class Node;
class Way;
class Relation;
}

namespace osrm
{

namespace util
{
struct Coordinate;
}

namespace extractor
{

class RestrictionParser;
class ExtractionRelationContainer;
struct ExtractionNode;
struct ExtractionWay;
struct ExtractionRelation;
struct ExtractionTurn;
struct ExtractionSegment;

/**
 * Abstract class that handles processing osmium ways, nodes and relation objects by applying
 * user supplied profiles.
 */
class ScriptingEnvironment
{
  public:
    ScriptingEnvironment() = default;
    ScriptingEnvironment(const ScriptingEnvironment &) = delete;
    ScriptingEnvironment &operator=(const ScriptingEnvironment &) = delete;
    virtual ~ScriptingEnvironment() = default;

    virtual const ProfileProperties &GetProfileProperties() = 0;

    virtual std::vector<std::vector<std::string>> GetExcludableClasses() = 0;
    virtual std::vector<std::string> GetClassNames() = 0;
    virtual std::vector<std::string> GetNameSuffixList() = 0;
    virtual std::vector<std::string> GetRestrictions() = 0;
    virtual void ProcessTurn(ExtractionTurn &turn) = 0;
    virtual void ProcessSegment(ExtractionSegment &segment) = 0;

    virtual void ProcessElements(
        const osmium::memory::Buffer &buffer,
        const RestrictionParser &restriction_parser,
        const ExtractionRelationContainer &relations,
        std::vector<std::pair<const osmium::Node &, ExtractionNode>> &resulting_nodes,
        std::vector<std::pair<const osmium::Way &, ExtractionWay>> &resulting_ways,
        std::vector<std::pair<const osmium::Relation &, ExtractionRelation>> &resulting_relations,
        std::vector<InputConditionalTurnRestriction> &resulting_restrictions) = 0;

    virtual bool HasLocationDependentData() const = 0;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_HPP */
