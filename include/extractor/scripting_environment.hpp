#ifndef SCRIPTING_ENVIRONMENT_HPP
#define SCRIPTING_ENVIRONMENT_HPP

#include "extractor/area/area_manager.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/obstacles.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/restriction.hpp"

#include <osmium/memory/buffer.hpp>

#include <boost/optional/optional.hpp>

#include <string>
#include <vector>

namespace osmium
{
class Node;
class Way;
class Relation;
} // namespace osmium

namespace osrm
{

namespace util
{
struct Coordinate;
} // namespace util

namespace extractor
{

class RestrictionParser;
class ManeuverOverrideRelationParser;
class ExtractionRelationContainer;
struct ExtractionTurn;
struct ExtractionSegment;

// The data structure that is passed through the parallel pipeline.
struct ScriptingResults
{
    // this keeps the buffer alive as long as the pipeline runs
    std::shared_ptr<osmium::memory::Buffer> osmium_buffer;
    std::vector<std::pair<const osmium::Node &, ExtractionNode>> resulting_nodes;
    std::vector<std::pair<const osmium::Way &, ExtractionWay>> resulting_ways;
    std::vector<InputTurnRestriction> resulting_restrictions;
    std::vector<InputManeuverOverride> resulting_maneuver_overrides;
};

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
    virtual bool HasRelationFunction() = 0;

    virtual std::vector<std::vector<std::string>> GetExcludableClasses() = 0;
    virtual std::vector<std::string> GetClassNames() = 0;
    virtual std::vector<std::string> GetNameSuffixList() = 0;
    virtual std::vector<std::string> GetRestrictions() = 0;
    virtual std::vector<std::string> GetRelations() = 0;
    virtual void ProcessTurn(ExtractionTurn &turn) = 0;
    virtual void ProcessSegment(ExtractionSegment &segment) = 0;

    virtual void ProcessRelation(const osmium::memory::Buffer &buffer,
                                 const ExtractionRelationContainer &relations,
                                 ScriptingResults &result_buffer) = 0;
    virtual void ProcessElements(const osmium::memory::Buffer &buffer,
                                 const RestrictionParser &restriction_parser,
                                 const ManeuverOverrideRelationParser &maneuver_override_parser,
                                 const ExtractionRelationContainer &relations,
                                 ScriptingResults &result_buffer) = 0;

    virtual bool HasLocationDependentData() const = 0;

    ObstacleMap m_obstacle_map; // The obstacle map shared by all threads.
    area::AreaManager m_area_manager;
};
} // namespace extractor
} // namespace osrm

#endif /* SCRIPTING_ENVIRONMENT_HPP */
