#ifndef SCRIPTING_ENVIRONMENT_LUA_HPP
#define SCRIPTING_ENVIRONMENT_LUA_HPP

#include "extractor/extraction_relation.hpp"
#include "extractor/location_dependent_data.hpp"
#include "extractor/raster_source.hpp"
#include "extractor/scripting_environment.hpp"

#include <tbb/enumerable_thread_specific.h>

#include <memory>
#include <mutex>
#include <string>

#include <sol2/sol.hpp>

namespace osrm
{
namespace extractor
{

struct LuaScriptingContext final
{
    LuaScriptingContext(const LocationDependentData &location_dependent_data)
        : location_dependent_data(location_dependent_data),
          last_location_point(0., 180.) // assume (0,180) is invalid coordinate
    {
    }

    void ProcessNode(const osmium::Node &,
                     ExtractionNode &result,
                     const ExtractionRelationContainer &relations);
    void ProcessWay(const osmium::Way &,
                    ExtractionWay &result,
                    const ExtractionRelationContainer &relations);

    ProfileProperties properties;
    RasterContainer raster_sources;
    sol::state state;

    bool has_turn_penalty_function;
    bool has_node_function;
    bool has_way_function;
    bool has_segment_function;

    sol::function turn_function;
    sol::function way_function;
    sol::function node_function;
    sol::function segment_function;

    int api_version;
    sol::table profile_table;

    // Reference to immutable location dependent data and locations memo
    const LocationDependentData &location_dependent_data;
    LocationDependentData::point_t last_location_point;
    std::vector<std::size_t> last_location_indexes;
};

/**
 * Creates a lua context and binds osmium way, node and relation objects and
 * ExtractionWay and ExtractionNode to lua objects.
 *
 * Each thread has its own lua state which is implemented with thread specific
 * storage from TBB.
 */
class Sol2ScriptingEnvironment final : public ScriptingEnvironment
{
  public:
    static const constexpr int SUPPORTED_MIN_API_VERSION = 0;
    static const constexpr int SUPPORTED_MAX_API_VERSION = 4;

    explicit Sol2ScriptingEnvironment(
        const std::string &file_name,
        const std::vector<boost::filesystem::path> &location_dependent_data_paths);
    ~Sol2ScriptingEnvironment() override = default;

    const ProfileProperties &GetProfileProperties() override;

    std::vector<std::vector<std::string>> GetExcludableClasses() override;
    std::vector<std::string> GetNameSuffixList() override;
    std::vector<std::string> GetClassNames() override;
    std::vector<std::string> GetRestrictions() override;
    std::vector<std::string> GetRelations() override;
    void ProcessTurn(ExtractionTurn &turn) override;
    void ProcessSegment(ExtractionSegment &segment) override;

    void
    ProcessElements(const osmium::memory::Buffer &buffer,
                    const RestrictionParser &restriction_parser,
                    const ManeuverOverrideRelationParser &maneuver_override_parser,
                    const ExtractionRelationContainer &relations,
                    std::vector<std::pair<const osmium::Node &, ExtractionNode>> &resulting_nodes,
                    std::vector<std::pair<const osmium::Way &, ExtractionWay>> &resulting_ways,
                    std::vector<InputConditionalTurnRestriction> &resulting_restrictions,
                    std::vector<InputManeuverOverride> &resulting_maneuver_overrides) override;

    bool HasLocationDependentData() const override { return !location_dependent_data.empty(); }

  private:
    LuaScriptingContext &GetSol2Context();

    std::vector<std::string> GetStringListFromTable(const std::string &table_name);
    std::vector<std::vector<std::string>> GetStringListsFromTable(const std::string &table_name);
    std::vector<std::string> GetStringListFromFunction(const std::string &function_name);

    void InitContext(LuaScriptingContext &context);
    std::mutex init_mutex;
    std::string file_name;
    tbb::enumerable_thread_specific<std::unique_ptr<LuaScriptingContext>> script_contexts;
    const LocationDependentData location_dependent_data;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_LUA_HPP */
