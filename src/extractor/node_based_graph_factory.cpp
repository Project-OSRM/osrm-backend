#include "extractor/node_based_graph_factory.hpp"
#include "extractor/files.hpp"
#include "extractor/graph_compressor.hpp"
#include "storage/io.hpp"

#include "util/log.hpp"
#include "util/timing_util.hpp"

#include <boost/assert.hpp>

#include <set>

namespace osrm::extractor
{

NodeBasedGraphFactory::NodeBasedGraphFactory(
    ScriptingEnvironment &scripting_environment,
    std::vector<TurnRestriction> &turn_restrictions,
    std::vector<UnresolvedManeuverOverride> &maneuver_overrides,
    TrafficSignals &traffic_signals,
    std::unordered_set<NodeID> &&barriers,
    std::vector<util::Coordinate> &&coordinates,
    extractor::PackedOSMIDs &&osm_node_ids,
    const std::vector<NodeBasedEdge> &edge_list,
    std::vector<NodeBasedEdgeAnnotation> &&annotation_data)
    : annotation_data(std::move(annotation_data)), barriers(std::move(barriers)),
      coordinates(std::move(coordinates)), osm_node_ids(std::move(osm_node_ids))
{
    BuildCompressedOutputGraph(edge_list);
    Compress(scripting_environment, turn_restrictions, maneuver_overrides, traffic_signals);
    CompressGeometry();
    CompressAnnotationData();
}

void NodeBasedGraphFactory::BuildCompressedOutputGraph(const std::vector<NodeBasedEdge> &edge_list)
{
    const auto number_of_node_based_nodes = coordinates.size();
    if (edge_list.empty())
    {
        throw util::exception("Node-based-graph contains no edges." + SOURCE_REF);
    }

    // at this point, the data isn't compressed, but since we update the graph in-place, we assign
    // it here.
    compressed_output_graph =
        util::NodeBasedDynamicGraphFromEdges(number_of_node_based_nodes, edge_list);

    // check whether the graph is sane
    BOOST_ASSERT(
        [this]()
        {
            for (const auto nbg_node_u :
                 util::irange(0u, compressed_output_graph.GetNumberOfNodes()))
            {
                for (EdgeID nbg_edge_id : compressed_output_graph.GetAdjacentEdgeRange(nbg_node_u))
                {
                    // we cannot have invalid edge-ids in the graph
                    if (nbg_edge_id == SPECIAL_EDGEID)
                        return false;

                    const auto nbg_node_v = compressed_output_graph.GetTarget(nbg_edge_id);

                    auto reverse = compressed_output_graph.FindEdge(nbg_node_v, nbg_node_u);

                    // found an edge that is reversed in both directions, should be two distinct
                    // edges
                    if (compressed_output_graph.GetEdgeData(nbg_edge_id).reversed &&
                        compressed_output_graph.GetEdgeData(reverse).reversed)
                        return false;
                }
            }
            return true;
        }());
}

void NodeBasedGraphFactory::Compress(ScriptingEnvironment &scripting_environment,
                                     std::vector<TurnRestriction> &turn_restrictions,
                                     std::vector<UnresolvedManeuverOverride> &maneuver_overrides,
                                     TrafficSignals &traffic_signals)
{
    GraphCompressor graph_compressor;
    graph_compressor.Compress(barriers,
                              traffic_signals,
                              scripting_environment,
                              turn_restrictions,
                              maneuver_overrides,
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
    TIMER_START(compress_annotation);
    /** Main idea, that we need to remove duplicated and unreferenced data
     * For that:
     * 1. We create set, that contains indecies of unique data items. Just create
     * comparator, that compare data from annotation_data vector by passed index.
     * 2. Create cached id's unordered_map, where key - stored id in set,
     * value - index of item in a set from begin. We need that map, because
     * std::distance(set.begin(), it) is too slow O(N). So any words in that step we reorder
     * annotation data to the order it stored in a set. Apply new id's to edge data.
     * 3. Remove unused anootation_data items.
     * 4. At final step just need to sort result annotation_data in the same order as set.
     * That makes id's stored in edge data valid.
     */
    struct IndexComparator
    {
        IndexComparator(const std::vector<NodeBasedEdgeAnnotation> &annotation_data_)
            : annotation_data(annotation_data_)
        {
        }

        bool operator()(AnnotationID a, AnnotationID b) const
        {
            return annotation_data[a] < annotation_data[b];
        }

      private:
        const std::vector<NodeBasedEdgeAnnotation> &annotation_data;
    };

    /** 1 */
    IndexComparator comparator(annotation_data);
    std::set<AnnotationID, IndexComparator> unique_annotations(comparator);

    // first we mark entries, by setting their mapping to 0
    for (const auto nbg_node_u : util::irange(0u, compressed_output_graph.GetNumberOfNodes()))
    {
        BOOST_ASSERT(nbg_node_u != SPECIAL_NODEID);
        for (EdgeID nbg_edge_id : compressed_output_graph.GetAdjacentEdgeRange(nbg_node_u))
        {
            auto const &edge = compressed_output_graph.GetEdgeData(nbg_edge_id);
            unique_annotations.insert(edge.annotation_data);
        }
    }

    // make additional map, because std::distance of std::set seems is O(N)
    // that very slow
    /** 2 */
    AnnotationID new_id = 0;
    std::unordered_map<AnnotationID, AnnotationID> cached_ids;
    for (auto id : unique_annotations)
        cached_ids[id] = new_id++;

    // apply the mapping
    for (const auto nbg_node_u : util::irange(0u, compressed_output_graph.GetNumberOfNodes()))
    {
        BOOST_ASSERT(nbg_node_u != SPECIAL_NODEID);
        for (EdgeID nbg_edge_id : compressed_output_graph.GetAdjacentEdgeRange(nbg_node_u))
        {
            auto &edge = compressed_output_graph.GetEdgeData(nbg_edge_id);
            auto const it = unique_annotations.find(edge.annotation_data);
            BOOST_ASSERT(it != unique_annotations.end());
            auto const it2 = cached_ids.find(*it);
            BOOST_ASSERT(it2 != cached_ids.end());

            edge.annotation_data = it2->second;
        }
    }

    /** 3 */
    // mark unused references for remove
    for (AnnotationID id = 0; id < annotation_data.size(); ++id)
    {
        auto const it = unique_annotations.find(id);
        if (it == unique_annotations.end() || *it != id)
            annotation_data[id].name_id = INVALID_NAMEID;
    }

    // remove unreferenced entries, shifting other entries to the front
    const auto new_end = std::remove_if(annotation_data.begin(),
                                        annotation_data.end(),
                                        [&](auto const &data)
                                        {
                                            // both elements are considered equal (to remove the
                                            // second one) if the annotation mapping of the second
                                            // one is invalid
                                            return data.name_id == INVALID_NAMEID;
                                        });

    const auto old_size = annotation_data.size();
    // remove all remaining elements
    annotation_data.erase(new_end, annotation_data.end());

    // reorder data in the same order
    /** 4 */
    std::sort(annotation_data.begin(), annotation_data.end());

    TIMER_STOP(compress_annotation);
    util::Log() << " graph compression removed " << (old_size - annotation_data.size())
                << " annotations of " << old_size << " in " << TIMER_SEC(compress_annotation)
                << " seconds";
}

void NodeBasedGraphFactory::ReleaseOsmNodes()
{
    // replace with a new vector to release old memory
    extractor::PackedOSMIDs().swap(osm_node_ids);
}

} // namespace osrm::extractor
