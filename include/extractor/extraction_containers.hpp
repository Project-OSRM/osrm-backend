#ifndef EXTRACTION_CONTAINERS_HPP
#define EXTRACTION_CONTAINERS_HPP

#include "extractor/first_and_last_segment_of_way.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction.hpp"
#include "extractor/scripting_environment.hpp"

#include "storage/io.hpp"

#include <cstdint>
#include <unordered_map>

#if USE_STXXL_LIBRARY
#include <stxxl/vector>
#endif

namespace osrm
{
namespace extractor
{

/**
 * Uses external memory containers from stxxl to store all the data that
 * is collected by the extractor callbacks.
 *
 * The data is the filtered, aggregated and finally written to disk.
 */
class ExtractionContainers
{
#if USE_STXXL_LIBRARY
    template <typename T> using ExternalVector = stxxl::vector<T>;
#else
    template <typename T> using ExternalVector = std::vector<T>;
#endif

    void FlushVectors();
    void PrepareNodes();
    void PrepareRestrictions();
    void PrepareEdges(ScriptingEnvironment &scripting_environment);

    void WriteNodes(storage::io::FileWriter &file_out) const;
    void WriteRestrictions(const std::string &restrictions_file_name);
    void WriteEdges(storage::io::FileWriter &file_out) const;
    void WriteCharData(const std::string &file_name);

  public:
    using NodeIDVector = ExternalVector<OSMNodeID>;
    using NodeVector = ExternalVector<QueryNode>;
    using EdgeVector = ExternalVector<InternalExtractorEdge>;
    using RestrictionsVector = std::vector<InputRestrictionContainer>;
    using WayIDStartEndVector = ExternalVector<FirstAndLastSegmentOfWay>;
    using NameCharData = ExternalVector<unsigned char>;
    using NameOffsets = ExternalVector<unsigned>;

    std::vector<OSMNodeID> barrier_nodes;
    std::vector<OSMNodeID> traffic_lights;
    NodeIDVector used_node_id_list;
    NodeVector all_nodes_list;
    EdgeVector all_edges_list;
    NameCharData name_char_data;
    NameOffsets name_offsets;
    // an adjacency array containing all turn lane masks
    RestrictionsVector restrictions_list;
    WayIDStartEndVector way_start_end_id_list;
    unsigned max_internal_node_id;
    std::vector<TurnRestriction> unconditional_turn_restrictions;

    ExtractionContainers();

    void PrepareData(ScriptingEnvironment &scripting_environment,
                     const std::string &output_file_name,
                     const std::string &restrictions_file_name,
                     const std::string &names_file_name);
};
}
}

#endif /* EXTRACTION_CONTAINERS_HPP */
