#ifndef SCRIPTING_ENVIRONMENT_HPP
#define SCRIPTING_ENVIRONMENT_HPP

#include "extractor/area/area_manager.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_way.hpp"
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

// The data structure that is passed through the tbb::parallel pipeline.
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
    ScriptingEnvironment() : m_area_manager(m_relations_stash){};
    ScriptingEnvironment(const ScriptingEnvironment &) = delete;
    ScriptingEnvironment &operator=(const ScriptingEnvironment &) = delete;
    virtual ~ScriptingEnvironment() = default;

    virtual const ProfileProperties &GetProfileProperties() = 0;

    virtual std::vector<std::vector<std::string>> GetExcludableClasses() = 0;
    virtual std::vector<std::string> GetClassNames() = 0;
    virtual std::vector<std::string> GetNameSuffixList() = 0;
    virtual std::vector<std::string> GetRestrictions() = 0;
    virtual std::vector<std::string> GetRelations() = 0;
    virtual void ProcessTurn(ExtractionTurn &turn) = 0;
    virtual void ProcessSegment(ExtractionSegment &segment) = 0;

    virtual void ProcessRelation(ScriptingResults &result_buffer) = 0;
    virtual void
    ProcessElements(ScriptingResults &result_buffer,
                    const RestrictionParser &restriction_parser,
                    const ManeuverOverrideRelationParser &maneuver_override_parser) = 0;

    virtual bool HasLocationDependentData() const = 0;

    /** The `relations` parameter to @ref process_way etc. */
    ExtractionRelationContainer m_relations_stash;
    /** The `obstacle map` global shared by all threads. */
    ObstacleMap m_obstacle_map;
    /** The `area_manager` global shared by all threads. */
    area::AreaManager m_area_manager;
};
} // namespace extractor
} // namespace osrm

#endif /* SCRIPTING_ENVIRONMENT_HPP */
