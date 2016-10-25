#ifndef SCRIPTING_ENVIRONMENT_SOL2_HPP
#define SCRIPTING_ENVIRONMENT_SOL2_HPP

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

struct Sol2ScriptingContext final
{
    void processNode(const osmium::Node &, ExtractionNode &result);
    void processWay(const osmium::Way &, ExtractionWay &result);

    ProfileProperties properties;
    SourceContainer sources;
    sol::state state;

    bool has_turn_penalty_function;
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
class Sol2ScriptingEnvironment final : public ScriptingEnvironment
{
  public:
    explicit Sol2ScriptingEnvironment(const std::string &file_name);
    ~Sol2ScriptingEnvironment() override = default;

    const ProfileProperties &GetProfileProperties() override;

    Sol2ScriptingContext &GetSol2Context();

    std::vector<std::string> GetNameSuffixList() override;
    std::vector<std::string> GetRestrictions() override;
    void SetupSources() override;
    int32_t GetTurnPenalty(double angle) override;
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
    void InitContext(Sol2ScriptingContext &context);
    std::mutex init_mutex;
    std::string file_name;
    tbb::enumerable_thread_specific<std::unique_ptr<Sol2ScriptingContext>> script_contexts;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_SOL2_HPP */
