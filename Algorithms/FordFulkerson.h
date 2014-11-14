// C++ program for finding minimum cut using Ford-Fulkerson

#include "../Util/simple_logger.hpp"
#include "../typedefs.h"

#include <boost/range/irange.hpp>

#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <stack>
#include <vector>

/*
  Returns true if there is a path from source 's' to sink 't' in
  residual graph. Also fills parent[] to store the path
  */
template<class GraphT>
class FordFulkerson
{
    public:
    bool bfs(std::shared_ptr<GraphT> &graph, const NodeID s, const NodeID t, std::vector<NodeID> &parent)
    {
        // Create a visited array and mark all vertices as not visited
        std::vector<bool> visited(graph->GetNumberOfNodes(), false);

        // Create a queue, enqueue source vertex and mark source vertex
        // as visited
        std::queue <NodeID> bfs_queue;
        bfs_queue.push(s);
        visited[s] = true;

        // mark target as unvisited as the parent pointer vector gets recycled
        visited[t] = false;
        parent[s] = -1;

        // Standard BFS Loop
        while (!bfs_queue.empty())
        {
            const NodeID u = bfs_queue.front();
            bfs_queue.pop();

            for (const EdgeID current_edge : graph->GetAdjacentEdgeRange(u))
            {
                const NodeID v = graph->GetTarget(current_edge);
                if (!visited[v] && graph->GetEdgeData(current_edge).distance > 0)
                {
                    bfs_queue.push(v);
                    parent[v] = u;
                    visited[v] = true;
                }
            }
        }

        // If we reached sink in BFS starting from source, then return
        // true, else false
        return visited[t];
    }

    // A DFS based function to find all reachable vertices from s.  The function
    // marks visited[i] as true if i is reachable from s.  The initial values in
    // visited[] must be false. We can also use BFS to find reachable vertices
    void dfs(std::shared_ptr<GraphT> &graph, const NodeID source, std::vector<bool> & visited)
    {
        std::stack<NodeID> dfs_stack;
        dfs_stack.push(source);

        while (!dfs_stack.empty())
        {
            const NodeID current_node = dfs_stack.top();
            dfs_stack.pop();
            visited[current_node] = true;
            // SimpleLogger().Write() << "popped node " << current_node;

            for (const EdgeID current_edge : graph->GetAdjacentEdgeRange(current_node))
            {
                // SimpleLogger().Write() << "checking edge" << current_edge;
                if (graph->GetEdgeData(current_edge).distance > 0)
                {
                    const NodeID v = graph->GetTarget(current_edge);
                    if (!visited[v])
                    {
                        dfs_stack.push(v);
                        // SimpleLogger().Write() << "dfs push " << v;
                    }
                }
            }
        }
    }

    // Prints the minimum s-t cut
    void minCut(std::shared_ptr<GraphT> &graph, const NodeID s, const NodeID t)
    {
        // Create a residual graph and fill the residual graph with
        // given capacities in the original graph as residual capacities
        // in residual graph

        std::vector<NodeID> parent(graph->GetNumberOfNodes());  // This array is filled by BFS and to store path

        // Augment the flow while tere is path from source to sink
        while (bfs(graph, s, t, parent))
        {
            SimpleLogger().Write() << "found path from " << s << " to " << t;
            // Find minimum residual capacity of the edhes along the
            // path filled by BFS. Or we can say find the maximum flow
            // through the path found.
            EdgeWeight path_flow = std::numeric_limits<EdgeWeight>::max();
            for (NodeID v=t; v!=s; v=parent[v])
            {
                const NodeID u = parent[v];
                const EdgeID current_edge = graph->FindEdge(u,v);
                path_flow = std::min(path_flow, graph->GetEdgeData(current_edge).distance);
            }
            SimpleLogger().Write() << "path has capacity " << path_flow;

            // update residual capacities of the edges and reverse edges
            // along the path
            for (NodeID v=t; v != s; v=parent[v])
            {
                const NodeID u = parent[v];
                const EdgeID forward_edge = graph->FindEdge(u,v);
                const EdgeID reverse_edge = graph->FindEdge(v,u);
                graph->GetEdgeData(forward_edge).distance -= path_flow;
                graph->GetEdgeData(reverse_edge).distance -= path_flow;
                SimpleLogger().Write() << "marked edge (" << u << "," << v << ")";
            }
            SimpleLogger().Write() << "Subtracted " << path_flow << " from path";
        }

        SimpleLogger().Write() << "extracting cut";

        // Flow is maximum now, find vertices reachable from s
        std::vector<bool> visited(graph->GetNumberOfNodes(), false);
        // bool visited[V];
        // memset(visited, false, sizeof(visited));
        dfs(graph, s, visited);

        SimpleLogger().Write() << "finished dfs";

        // Print all edges that are from a reachable vertex to
        // non-reachable vertex in the original graph
        for (const NodeID u : boost::irange(0u, graph->GetNumberOfNodes()))
        {
            for (const EdgeID current_edge : graph->GetAdjacentEdgeRange(u))
            {
                const NodeID v = graph->GetTarget(current_edge);
                if (visited[u] && !visited[v])
                {
                    std::cout << "cut edge (" << u << "," << v << ")" << std::endl;
                }
            }
        }
        return;
    }
};
