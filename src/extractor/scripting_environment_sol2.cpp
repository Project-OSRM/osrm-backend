#include "extractor/scripting_environment_sol2.hpp"

#include "extractor/external_memory_node.hpp"
#include "extractor/extraction_helper_functions.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/raster_source.hpp"
#include "extractor/restriction_parser.hpp"
#include "util/exception.hpp"
#include "util/lua_util.hpp"
#include "util/simple_logger.hpp"
#include "util/typedefs.hpp"

#include <osmium/osm.hpp>

#include <tbb/parallel_for.h>

#include <memory>
#include <sstream>

namespace osrm
{
namespace extractor
{

Sol2ScriptingEnvironment::Sol2ScriptingEnvironment(const std::string &file_name)
    : file_name(file_name)
{
    util::SimpleLogger().Write() << "Using script " << file_name;
}

void Sol2ScriptingEnvironment::InitContext(Sol2ScriptingContext &context)
{
    context.state.open_libraries();

    // TODO: register things here

    util::luaAddScriptFolderToLoadPath(context.state.lua_state(), file_name.c_str());

    context.has_turn_penalty_function = true;
    context.has_node_function = true;
    context.has_way_function = true;
    context.has_segment_function = true;
}

const ProfileProperties &Sol2ScriptingEnvironment::GetProfileProperties()
{
    return GetSol2Context().properties;
}

Sol2ScriptingContext &Sol2ScriptingEnvironment::GetSol2Context()
{
    std::lock_guard<std::mutex> lock(init_mutex);
    bool initialized = false;
    auto &ref = script_contexts.local(initialized);
    if (!initialized)
    {
        ref = std::make_unique<Sol2ScriptingContext>();
        InitContext(*ref);
    }

    return *ref;
}

void Sol2ScriptingEnvironment::ProcessElements(
    const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
    const RestrictionParser &restriction_parser,
    tbb::concurrent_vector<std::pair<std::size_t, ExtractionNode>> &resulting_nodes,
    tbb::concurrent_vector<std::pair<std::size_t, ExtractionWay>> &resulting_ways,
    tbb::concurrent_vector<boost::optional<InputRestrictionContainer>> &resulting_restrictions)
{
    // parse OSM entities in parallel, store in resulting vectors
    tbb::parallel_for(
        tbb::blocked_range<std::size_t>(0, osm_elements.size()),
        [&](const tbb::blocked_range<std::size_t> &range) {
            ExtractionNode result_node;
            ExtractionWay result_way;
            auto &local_context = this->GetSol2Context();

            for (auto x = range.begin(), end = range.end(); x != end; ++x)
            {
                const auto entity = osm_elements[x];

                switch (entity->type())
                {
                case osmium::item_type::node:
                    result_node.clear();
                    if (local_context.has_node_function)
                    {
                        local_context.processNode(static_cast<const osmium::Node &>(*entity),
                                                  result_node);
                    }
                    resulting_nodes.push_back(std::make_pair(x, std::move(result_node)));
                    break;
                case osmium::item_type::way:
                    result_way.clear();
                    if (local_context.has_way_function)
                    {
                        local_context.processWay(static_cast<const osmium::Way &>(*entity),
                                                 result_way);
                    }
                    resulting_ways.push_back(std::make_pair(x, std::move(result_way)));
                    break;
                case osmium::item_type::relation:
                    resulting_restrictions.push_back(restriction_parser.TryParse(
                        static_cast<const osmium::Relation &>(*entity)));
                    break;
                default:
                    break;
                }
            }
        });
}

std::vector<std::string> Sol2ScriptingEnvironment::GetNameSuffixList()
{
    auto &context = GetSol2Context();
    BOOST_ASSERT(context.state != nullptr);
    std::vector<std::string> suffixes_vector;
    // TODO: fill if get_suff. function exists
    return suffixes_vector;
}

std::vector<std::string> Sol2ScriptingEnvironment::GetRestrictions()
{
    auto &context = GetSol2Context();
    BOOST_ASSERT(context.state != nullptr);
    std::vector<std::string> restrictions;
    // TODO: fill if get_restrictoins is available
    return restrictions;
}

void Sol2ScriptingEnvironment::SetupSources()
{
    auto &context = GetSol2Context();
    BOOST_ASSERT(context.state != nullptr);

    // TODO: call source_function if exists
}

int32_t Sol2ScriptingEnvironment::GetTurnPenalty(const double angle)
{
    auto &context = GetSol2Context();
    // turn_function(angle) if function exists
    return 0;
}

void Sol2ScriptingEnvironment::ProcessSegment(const osrm::util::Coordinate &source,
                                              const osrm::util::Coordinate &target,
                                              double distance,
                                              InternalExtractorEdge::WeightData &weight)
{
    auto &context = GetSol2Context();
    // TODO: call segment_function if exists
}

void Sol2ScriptingContext::processNode(const osmium::Node &node, ExtractionNode &result)
{
    BOOST_ASSERT(state != nullptr);
    // TODO: node_function
}

void Sol2ScriptingContext::processWay(const osmium::Way &way, ExtractionWay &result)
{
    BOOST_ASSERT(state != nullptr);
    // TODO: way_function
}
}
}
