#ifndef SCRIPTING_ENVIRONMENT_LUA_HPP
#define SCRIPTING_ENVIRONMENT_LUA_HPP

#include "extractor/scripting_environment.hpp"
#include "extractor/turn_penalty.hpp"

#include "extractor/raster_source.hpp"

#include "util/lua_util.hpp"

#include <tbb/enumerable_thread_specific.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

struct lua_State;

namespace osrm
{
namespace extractor
{

struct LuaScriptingContext final
{
    void processNode(const osmium::Node &, ExtractionNode &result);
    void processWay(const osmium::Way &, ExtractionWay &result);

    ProfileProperties properties;
    SourceContainer sources;
    util::LuaState state;

    bool has_turn_penalty_function;
    bool has_detailed_penalty_function;
    bool has_node_function;
    bool has_way_function;
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

    const ProfileProperties &GetProfileProperties() override;

    LuaScriptingContext &GetLuaContext();

    std::vector<std::string> GetNameSuffixList() override;
    std::vector<std::string> GetRestrictions() override;
    void SetupSources() override;
    std::int32_t GetTurnPenalty(const TurnProperties &turn_properties,
                                const IntersectionProperties &intersection_properties,
                                const TurnSegment &approach_segment,
                                const TurnSegment &exit_segment) override;
    void ProcessSegment(const osrm::util::Coordinate &source,
                        const osrm::util::Coordinate &target,
                        double distance,
                        InternalExtractorEdge::WeightData &weight) override;
    void
    ProcessElements(const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
                    const RestrictionParser &restriction_parser,
                    tbb::concurrent_vector<std::pair<std::size_t, ExtractionNode>> &resulting_nodes,
                    tbb::concurrent_vector<std::pair<std::size_t, ExtractionWay>> &resulting_ways,
                    tbb::concurrent_vector<boost::optional<InputRestrictionContainer>>
                        &resulting_restrictions) override;

  private:
    void InitContext(LuaScriptingContext &context);
    std::mutex init_mutex;
    std::string file_name;
    tbb::enumerable_thread_specific<std::unique_ptr<LuaScriptingContext>> script_contexts;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_LUA_HPP */
