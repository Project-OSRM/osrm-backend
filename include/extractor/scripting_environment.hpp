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

#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/dynamic_handler.hpp>

namespace osmium
{
class Node;
class Way;
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
struct ExtractionNode;
struct ExtractionWay;

using index_sparse_type = osmium::index::map::SparseFileArray<osmium::unsigned_object_id_type, osmium::Location>;
using index_dense_type = osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, osmium::Location>;

// The location handler always depends on the index type
using location_handler_sparse_type = osmium::handler::NodeLocationsForWays<index_sparse_type>;
using location_handler_dense_type = osmium::handler::NodeLocationsForWays<index_dense_type>;

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

    virtual std::vector<std::string> GetNameSuffixList() = 0;
    virtual std::vector<std::string> GetRestrictions() = 0;
    virtual void SetupSources() = 0;
    virtual int32_t GetTurnPenalty(double angle) = 0;
    virtual void ProcessSegment(const osrm::util::Coordinate &source,
                                const osrm::util::Coordinate &target,
                                double distance,
                                InternalExtractorEdge::WeightData &weight) = 0;
    virtual void
    ProcessElements(const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
                    const RestrictionParser &restriction_parser,
                    tbb::concurrent_vector<std::pair<std::size_t, ExtractionNode>> &resulting_nodes,
                    tbb::concurrent_vector<std::pair<std::size_t, ExtractionWay>> &resulting_ways,
                    tbb::concurrent_vector<boost::optional<InputRestrictionContainer>>
                        &resulting_restrictions) = 0;

    static location_handler_sparse_type *location_sparse_handler;
    static location_handler_dense_type *location_dense_handler;
    static void setSparseCache(location_handler_sparse_type &cache) {
        location_sparse_handler = &cache;};
    static void setDenseCache(location_handler_dense_type &cache) {
        location_dense_handler = &cache;};
};
}
}
#endif /* SCRIPTING_ENVIRONMENT_HPP */
