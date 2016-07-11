#ifndef SCRIPTING_ENVIRONMENT_LUA_HPP
#define SCRIPTING_ENVIRONMENT_LUA_HPP

#include "extractor/scripting_environment.hpp"

#include "extractor/raster_source.hpp"

#include "util/lua_util.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <tbb/enumerable_thread_specific.h>

struct lua_State;

namespace osrm
{
namespace extractor
{

struct LuaScriptingContext final : public ScriptingContext
{
    LuaScriptingContext() = default;
    ~LuaScriptingContext() override = default;

    std::unordered_set<std::string> getNameSuffixList() override;
    std::vector<std::string> getExceptions() override;
    void setupSources() override;
    int getTurnPenalty(double angle) override;
    void processNode(const osmium::Node &, ExtractionNode &result) override;
    void processWay(const osmium::Way &, ExtractionWay &result) override;
    void processSegment(const osrm::util::Coordinate &source,
                        const osrm::util::Coordinate &target,
                        double distance,
                        InternalExtractorEdge::WeightData &weight) override;

    SourceContainer sources;
    util::LuaState state;

    bool has_turn_penalty_function;
    bool has_segment_function;
};

/**
 * Creates a lua context and binds osmium way, node and relation objects and
 * ExtractionWay and ExtractionNode to lua objects.
 *
 * Each thread has its own lua state which is implemented with thread specific
 * storage from TBB.
 */
class LuaScriptingEnvironment final : public ScriptingEnvironment
{
  public:
    explicit LuaScriptingEnvironment(const std::string &file_name);
    ~LuaScriptingEnvironment() override = default;

    ScriptingContext &GetContext() override;

  private:
    void InitContext(LuaScriptingContext &context);
    std::mutex init_mutex;
    std::string file_name;
    tbb::enumerable_thread_specific<std::unique_ptr<LuaScriptingContext>> script_contexts;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_LUA_HPP */
