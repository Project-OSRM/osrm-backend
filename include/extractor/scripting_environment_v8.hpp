#ifndef SCRIPTING_ENVIRONMENT_V8_HPP
#define SCRIPTING_ENVIRONMENT_V8_HPP

#include "extractor/scripting_environment.hpp"

#include "extractor/raster_source.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace v8
{
class Platform;
}

namespace osrm
{
namespace extractor
{

struct V8ScriptingContext;

/**
 * Creates a V8 context and binds osmium way, node and relation objects and
 * ExtractionWay and ExtractionNode to JS objects.
 *
 * Each thread has its own V8 state which is implemented with thread specific
 * storage from TBB.
 */
class V8ScriptingEnvironment final : public ScriptingEnvironment
{
  public:
    explicit V8ScriptingEnvironment(const char* argv0, const std::string &file_name);
    ~V8ScriptingEnvironment() override;

    const ProfileProperties& GetProfileProperties() override;

    V8ScriptingContext &GetV8Context();

    std::vector<std::string> GetNameSuffixList() override;
    std::vector<std::string> GetRestrictions() override;
    std::vector<std::string> GetExceptions();
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
    std::string file_name;
    v8::Platform *platform;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_V8_HPP */
