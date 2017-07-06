#ifndef SCRIPTING_ENVIRONMENT_LUA_HPP
#define SCRIPTING_ENVIRONMENT_LUA_HPP

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
    void ProcessNode(const osmium::Node &, ExtractionNode &result);
    void ProcessWay(const osmium::Way &, ExtractionWay &result);

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
    static const constexpr int SUPPORTED_MAX_API_VERSION = 2;

    explicit Sol2ScriptingEnvironment(const std::string &file_name);
    ~Sol2ScriptingEnvironment() override = default;

    const ProfileProperties &GetProfileProperties() override;

    LuaScriptingContext &GetSol2Context();

    std::vector<std::string> GetStringListFromTable(const std::string &table_name);
    std::vector<std::string> GetStringListFromFunction(const std::string &function_name);
    std::vector<std::string> GetNameSuffixList() override;
    std::vector<std::string> GetRestrictions() override;
    void ProcessTurn(ExtractionTurn &turn) override;
    void ProcessSegment(ExtractionSegment &segment) override;

    void
    ProcessElements(const osmium::memory::Buffer &buffer,
                    const RestrictionParser &restriction_parser,
                    std::vector<std::pair<const osmium::Node &, ExtractionNode>> &resulting_nodes,
                    std::vector<std::pair<const osmium::Way &, ExtractionWay>> &resulting_ways,
                    std::vector<InputConditionalTurnRestriction> &resulting_restrictions) override;

  private:
    void InitContext(LuaScriptingContext &context);
    std::mutex init_mutex;
    std::string file_name;
    tbb::enumerable_thread_specific<std::unique_ptr<LuaScriptingContext>> script_contexts;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_LUA_HPP */
