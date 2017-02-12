#ifndef PEARCE_SCC_HPP
#define PEARCE_SCC_HPP

#include "extractor/node_based_edge.hpp"
#include "extractor/query_node.hpp"
#include "util/deallocating_vector.hpp"
#include "util/percent.hpp"
#include "util/typedefs.hpp"

#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/std_hash.hpp"
#include "util/timing_util.hpp"

#include "osrm/coordinate.hpp"
#include <boost/assert.hpp>
#include <cstdint>

#include <algorithm>
#include <climits>
#include <memory>
#include <stack>
#include <vector>

namespace osrm
{
namespace extractor
{

template <typename GraphT> class PearceSCC
{
    std::vector<NodeID> components_index;
    std::vector<std::size_t> component_size_vector;
    const GraphT &m_graph;

    struct StackFrame
    {
        NodeID v;
        typename GraphT::EdgeRange edges;
        bool root;
    };

  public:
    PearceSCC(const GraphT &graph)
        : components_index(graph.GetNumberOfNodes(), SPECIAL_NODEID), m_graph(graph)
    {
    }

    void Run()
    {
        TIMER_START(SCC_RUN);
        const NodeID max_node_id = m_graph.GetNumberOfNodes();

        // recursion stack contains local variables: v, (w, end(w)), root
        std::stack<StackFrame> recursion_stack;
        std::stack<NodeID> stack;
        std::vector<NodeID> rindex(max_node_id, 0);
        NodeID index = 1, rcomponent = max_node_id - 1;

        for (const auto u : util::irange(0u, max_node_id))
        {
            if (rindex[u] == 0)
            {
                // visit(u)
                rindex[u] = index;
                index += 1;
                recursion_stack.push({u, m_graph.GetAdjacentEdgeRange(u), true});
                while (!recursion_stack.empty())
                {
                    bool recursion_step = false;
                    auto &frame = recursion_stack.top();
                    const auto v = frame.v;
                    for (auto edge : frame.edges)
                    {
                        const auto w = m_graph.GetTarget(edge);
                        if (rindex[w] == 0)
                        {
                            // visit(w)
                            recursion_step = true;
                            rindex[w] = index;
                            index += 1;
                            recursion_stack.push({w, m_graph.GetAdjacentEdgeRange(w), true});
                            break;
                        }
                        else if (rindex[w] < rindex[v])
                        {
                            rindex[v] = rindex[w];
                            frame.root = false; // root = false
                        }
                    }

                    if (recursion_step)
                        continue;

                    if (frame.root)
                    {
                        std::size_t size_of_current_component = 1;
                        const NodeID component_index = component_size_vector.size();
                        components_index[v] = component_index;

                        index -= 1;
                        while (!stack.empty() && rindex[v] <= rindex[stack.top()])
                        {
                            const auto w = stack.top();
                            stack.pop();
                            rindex[w] = rcomponent;
                            index -= 1;

                            size_of_current_component += 1;
                            components_index[w] = component_index;
                        }
                        rindex[v] = rcomponent;
                        rcomponent -= 1;

                        component_size_vector.push_back(size_of_current_component);
                        if (component_size_vector.back() > 1000)
                        {
                            util::Log() << "large component [" << component_index
                                        << "]=" << component_size_vector.back();
                        }
                    }
                    else
                    {
                        stack.push(v);
                    }

                    recursion_stack.pop();
                }

                BOOST_ASSERT(stack.empty());
            }
        }

        TIMER_STOP(SCC_RUN);
        util::Log() << "SCC run took: " << TIMER_MSEC(SCC_RUN) / 1000. << "s";
    }

    std::size_t GetNumberOfComponents() const { return component_size_vector.size(); }

    std::size_t GetSizeOneCount() const
    {
        return std::count_if(component_size_vector.begin(),
                             component_size_vector.end(),
                             [](unsigned value) { return 1 == value; });
    }

    std::size_t GetComponentSize(const unsigned component_id) const
    {
        return component_size_vector[component_id];
    }

    NodeID GetComponentID(const NodeID node) const { return components_index[node]; }
};
}
}

// References:
// An Improved Algorithm for Finding the Strongly Connected Components of a Directed Graph,
// David J. Pearce, 2006. Algorithm 3

#endif /* PEARCE_SCC_HPP */
