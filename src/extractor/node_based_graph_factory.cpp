#include "extractor/node_based_graph_factory.hpp"
#include "extractor/graph_compressor.hpp"
#include "storage/io.hpp"
#include "util/graph_loader.hpp"

#include "util/log.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{

NodeBasedGraphFactory::NodeBasedGraphFactory(
    const boost::filesystem::path &input_file,
    ScriptingEnvironment &scripting_environment,
    std::vector<TurnRestriction> &turn_restrictions,
    std::vector<ConditionalTurnRestriction> &conditional_turn_restrictions)
{
    LoadDataFromFile(input_file);
    Compress(scripting_environment, turn_restrictions, conditional_turn_restrictions);
    CompressGeometry();
    CompressAnnotationData();
}

// load the data serialised during the extraction run
void NodeBasedGraphFactory::LoadDataFromFile(const boost::filesystem::path &input_file)
{
    // the extraction_containers serialise all data necessary to create the node-based graph into a
    // single file, the *.osrm file. It contains nodes, basic information about which of these nodes
    // are traffic signals/stop signs. It also contains Edges and purely annotative meta-data
    storage::io::FileReader file_reader(input_file, storage::io::FileReader::VerifyFingerprint);

    auto barriers_iter = inserter(barriers, end(barriers));
    auto traffic_signals_iter = inserter(traffic_signals, end(traffic_signals));

    const auto number_of_node_based_nodes = util::loadNodesFromFile(
        file_reader, barriers_iter, traffic_signals_iter, coordinates, osm_node_ids);

    std::vector<NodeBasedEdge> edge_list;
    util::loadEdgesFromFile(file_reader, edge_list);

    if (edge_list.empty())
    {
        throw util::exception("Node-based-graph (" + input_file.string() + ") contains no edges." +
                              SOURCE_REF);
    }

    util::loadAnnotationData(file_reader, annotation_data);

    // at this point, the data isn't compressed, but since we update the graph in-place, we assign
    // it here.
    compressed_output_graph =
        util::NodeBasedDynamicGraphFromEdges(number_of_node_based_nodes, edge_list);

    // check whether the graph is sane
    BOOST_ASSERT([this]() {
        for (const auto nbg_node_u : util::irange(0u, compressed_output_graph.GetNumberOfNodes()))
        {
            for (EdgeID nbg_edge_id : compressed_output_graph.GetAdjacentEdgeRange(nbg_node_u))
            {
                // we cannot have invalid edge-ids in the graph
                if (nbg_edge_id == SPECIAL_EDGEID)
                    return false;

                const auto nbg_node_v = compressed_output_graph.GetTarget(nbg_edge_id);

                auto reverse = compressed_output_graph.FindEdge(nbg_node_v, nbg_node_u);

                // found an edge that is reversed in both directions, should be two distinct edges
                if (compressed_output_graph.GetEdgeData(nbg_edge_id).reversed &&
                    compressed_output_graph.GetEdgeData(reverse).reversed)
                    return false;
            }
        }
        return true;
    }());
}

void NodeBasedGraphFactory::Compress(
    ScriptingEnvironment &scripting_environment,
    std::vector<TurnRestriction> &turn_restrictions,
    std::vector<ConditionalTurnRestriction> &conditional_turn_restrictions)
{
    GraphCompressor graph_compressor;
    graph_compressor.Compress(barriers,
                              traffic_signals,
                              scripting_environment,
                              turn_restrictions,
                              conditional_turn_restrictions,
                              compressed_output_graph,
                              annotation_data,
                              compressed_edge_container);
}

void NodeBasedGraphFactory::CompressGeometry()
{
    for (const auto nbg_node_u : util::irange(0u, compressed_output_graph.GetNumberOfNodes()))
    {
        for (EdgeID nbg_edge_id : compressed_output_graph.GetAdjacentEdgeRange(nbg_node_u))
        {
            BOOST_ASSERT(nbg_edge_id != SPECIAL_EDGEID);

            const auto &nbg_edge_data = compressed_output_graph.GetEdgeData(nbg_edge_id);
            const auto nbg_node_v = compressed_output_graph.GetTarget(nbg_edge_id);
            BOOST_ASSERT(nbg_node_v != SPECIAL_NODEID);
            BOOST_ASSERT(nbg_node_u != nbg_node_v);

            // pick only every other edge, since we have every edge as an outgoing
            // and incoming egde
            if (nbg_node_u >= nbg_node_v)
            {
                continue;
            }

            auto from = nbg_node_u, to = nbg_node_v;
            // if we found a non-forward edge reverse and try again
            if (nbg_edge_data.reversed)
                std::swap(from, to);

            // find forward edge id and
            const EdgeID edge_id_1 = compressed_output_graph.FindEdge(from, to);
            BOOST_ASSERT(edge_id_1 != SPECIAL_EDGEID);

            // find reverse edge id and
            const EdgeID edge_id_2 = compressed_output_graph.FindEdge(to, from);
            BOOST_ASSERT(edge_id_2 != SPECIAL_EDGEID);

            BOOST_ASSERT(compressed_edge_container.HasEntryForID(edge_id_1) ==
                     compressed_edge_container.HasEntryForID(edge_id_2));
            BOOST_ASSERT(compressed_edge_container.HasEntryForID(edge_id_1));
            BOOST_ASSERT(compressed_edge_container.HasEntryForID(edge_id_2));
            auto packed_geometry_id = compressed_edge_container.ZipEdges(edge_id_1, edge_id_2);

            // remember the geometry ID for both edges in the node-based graph
            compressed_output_graph.GetEdgeData(edge_id_1).geometry_id = {packed_geometry_id, true};
            compressed_output_graph.GetEdgeData(edge_id_2).geometry_id = {packed_geometry_id,
                                                                          false};
        }
    }
}

void NodeBasedGraphFactory::CompressAnnotationData()
{
    const constexpr AnnotationID INVALID_ANNOTATIONID = -1;
    // remap all entries to find which are used
    std::vector<AnnotationID> annotation_mapping(annotation_data.size(), INVALID_ANNOTATIONID);

    // first we mark entries, by setting their mapping to 0
    for (const auto nbg_node_u : util::irange(0u, compressed_output_graph.GetNumberOfNodes()))
    {
        BOOST_ASSERT(nbg_node_u != SPECIAL_NODEID);
        for (EdgeID nbg_edge_id : compressed_output_graph.GetAdjacentEdgeRange(nbg_node_u))
        {
            auto const &edge = compressed_output_graph.GetEdgeData(nbg_edge_id);
            annotation_mapping[edge.annotation_data] = 0;
        }
    }

    // now compute a prefix sum on all entries that are 0 to find the new mapping
    AnnotationID prefix_sum = 0;
    for (std::size_t i = 0; i < annotation_mapping.size(); ++i)
    {
        if (annotation_mapping[i] == 0)
            annotation_mapping[i] = prefix_sum++;
        else
        {
            // flag for removal
            annotation_data[i].name_id = INVALID_NAMEID;
        }
    }

    // apply the mapping
    for (const auto nbg_node_u : util::irange(0u, compressed_output_graph.GetNumberOfNodes()))
    {
        BOOST_ASSERT(nbg_node_u != SPECIAL_NODEID);
        for (EdgeID nbg_edge_id : compressed_output_graph.GetAdjacentEdgeRange(nbg_node_u))
        {
            auto &edge = compressed_output_graph.GetEdgeData(nbg_edge_id);
            edge.annotation_data = annotation_mapping[edge.annotation_data];
            BOOST_ASSERT(edge.annotation_data != INVALID_ANNOTATIONID);
        }
    }

    // remove unreferenced entries, shifting other entries to the front
    const auto new_end =
        std::remove_if(annotation_data.begin(), annotation_data.end(), [&](auto const &data) {
            // both elements are considered equal (to remove the second
            // one) if the annotation mapping of the second one is
            // invalid
            return data.name_id == INVALID_NAMEID;
        });

    const auto old_size = annotation_data.size();
    // remove all remaining elements
    annotation_data.erase(new_end, annotation_data.end());
    util::Log() << " graph compression removed " << (old_size - annotation_data.size())
                << " annotations of " << old_size;
}

void NodeBasedGraphFactory::ReleaseOsmNodes()
{
    // replace with a new vector to release old memory
    extractor::PackedOSMIDs().swap(osm_node_ids);
}

} // namespace extractor
} // namespace osrm
