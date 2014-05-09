#ifndef __NODE_BASED_GRAPH_H__
#define __NODE_BASED_GRAPH_H__

#include "DynamicGraph.h"
#include "ImportEdge.h"

#include <memory>

struct NodeBasedEdgeData
{
    NodeBasedEdgeData()
        : distance(INVALID_EDGE_WEIGHT), edgeBasedNodeID(SPECIAL_NODEID),
          nameID(std::numeric_limits<unsigned>::max()), type(std::numeric_limits<short>::max()),
          isAccessRestricted(false), shortcut(false), forward(false), backward(false),
          roundabout(false), ignore_in_grid(false), contraFlow(false)
    {
    }

    int distance;
    unsigned edgeBasedNodeID;
    unsigned nameID;
    short type;
    bool isAccessRestricted : 1;
    bool shortcut : 1;
    bool forward : 1;
    bool backward : 1;
    bool roundabout : 1;
    bool ignore_in_grid : 1;
    bool contraFlow : 1;

    void SwapDirectionFlags()
    {
        bool temp_flag = forward;
        forward = backward;
        backward = temp_flag;
    }

    bool IsEqualTo(const NodeBasedEdgeData &other) const
    {
        return (forward == other.forward) && (backward == other.backward) &&
               (nameID == other.nameID) && (ignore_in_grid == other.ignore_in_grid) &&
               (contraFlow == other.contraFlow);
    }
};

typedef DynamicGraph<NodeBasedEdgeData> NodeBasedDynamicGraph;

// Factory method to create NodeBasedDynamicGraph from ImportEdges
inline std::shared_ptr<NodeBasedDynamicGraph>
NodeBasedDynamicGraphFromImportEdges(int number_of_nodes, std::vector<ImportEdge> &input_edge_list)
{
    std::sort(input_edge_list.begin(), input_edge_list.end());

    // TODO: remove duplicate edges
    DeallocatingVector<NodeBasedDynamicGraph::InputEdge> edges_list;
    NodeBasedDynamicGraph::InputEdge edge;
    for (const ImportEdge &import_edge : input_edge_list)
    {
        // TODO: give ImportEdge a proper c'tor to use emplace_back's below
        if (!import_edge.isForward())
        {
            edge.source = import_edge.target();
            edge.target = import_edge.source();
            edge.data.backward = import_edge.isForward();
            edge.data.forward = import_edge.isBackward();
        }
        else
        {
            edge.source = import_edge.source();
            edge.target = import_edge.target();
            edge.data.forward = import_edge.isForward();
            edge.data.backward = import_edge.isBackward();
        }

        if (edge.source == edge.target)
        {
            continue;
        }

        edge.data.distance = (std::max)((int)import_edge.weight(), 1);
        BOOST_ASSERT(edge.data.distance > 0);
        edge.data.shortcut = false;
        edge.data.roundabout = import_edge.isRoundabout();
        edge.data.ignore_in_grid = import_edge.ignoreInGrid();
        edge.data.nameID = import_edge.name();
        edge.data.type = import_edge.type();
        edge.data.isAccessRestricted = import_edge.isAccessRestricted();
        edge.data.contraFlow = import_edge.isContraFlow();
        edges_list.push_back(edge);

        if (!import_edge.IsSplit())
        {
            using std::swap; // enable ADL
            swap(edge.source, edge.target);
            edge.data.SwapDirectionFlags();
            edges_list.push_back(edge);
        }
    }

    std::sort(edges_list.begin(), edges_list.end());
    auto graph = std::make_shared<NodeBasedDynamicGraph>(number_of_nodes, edges_list);

    edges_list.clear();
    BOOST_ASSERT(0 == edges_list.size());

    return graph;
}

#endif // __NODE_BASED_GRAPH_H__
