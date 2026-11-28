#include "partitioner/inertial_flow.hpp"
#include "partitioner/bisection_graph.hpp"
#include "partitioner/bisection_graph_view.hpp"
#include "partitioner/reorder_first_last.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

namespace osrm::partitioner
{
namespace
{
// Mark the n * ratio nodes at each end of the spatial order as terminals.
std::vector<DinicMaxFlow::Terminal>
makeTerminals(const BisectionGraphView &view, const double ratio, const double slope)
{
    struct NodeWithCoordinate
    {
        NodeWithCoordinate(NodeID nid_, const util::Coordinate &coordinate_)
            : nid{nid_}, coordinate{coordinate_}
        {
        }

        NodeID nid;
        util::Coordinate coordinate;
    };

    using Embedding = std::vector<NodeWithCoordinate>;

    Embedding embedding;
    embedding.reserve(view.NumberOfNodes());

    // adress of the very first node
    const auto node_zero = &(*view.Begin());

    std::transform(view.Begin(),
                   view.End(),
                   std::back_inserter(embedding),
                   [&](const auto &node)
                   {
                       const auto node_id = static_cast<NodeID>(&node - node_zero);
                       return NodeWithCoordinate{node_id, node.coordinate};
                   });

    const auto project = [slope](const auto &each)
    {
        auto lon = static_cast<std::int32_t>(each.coordinate.lon);
        auto lat = static_cast<std::int32_t>(each.coordinate.lat);

        return slope * lon + (1. - std::fabs(slope)) * lat;
    };

    const auto spatially = [&](const auto &lhs, const auto &rhs)
    { return project(lhs) < project(rhs); };

    const std::size_t n = ratio * embedding.size();

    reorderFirstLast(embedding, n, spatially);

    using Terminal = DinicMaxFlow::Terminal;
    std::vector<Terminal> terminals(embedding.size(), Terminal::None);
    for (auto it = begin(embedding), last = begin(embedding) + n; it != last; ++it)
        terminals[it->nid] = Terminal::Source;
    for (auto it = end(embedding) - n, last = end(embedding); it != last; ++it)
        terminals[it->nid] = Terminal::Sink;

    return terminals;
}

// Makes n cuts with different spatial orders and returns the best.
DinicMaxFlow::MinCut bestMinCut(const BisectionGraphView &view,
                                const std::size_t n,
                                const double ratio,
                                const double balance)
{
    DinicMaxFlow::MinCut best;
    best.num_edges = -1;

    const auto get_balance = [&view, balance](const std::size_t num_nodes_source)
    {
        const auto perfect_balance = view.NumberOfNodes() / 2;
        const auto allowed_balance = balance * perfect_balance;
        const auto bigger_side =
            std::max(num_nodes_source, view.NumberOfNodes() - num_nodes_source);

        if (bigger_side > allowed_balance)
            return bigger_side / static_cast<double>(allowed_balance);
        else
            return 1.0;
    };

    auto best_balance = 1;

    // The incumbent uses an integral balance factor. When every factor rounds to
    // one, an accepted cut can only decrease the incumbent edge count.
    const auto maximum_balance = get_balance(0);
    std::atomic<std::size_t> best_edge_count{std::numeric_limits<std::size_t>::max()};
    const auto cut_limit =
        maximum_balance >= 1.0 && maximum_balance < 2.0 ? &best_edge_count : nullptr;

    std::mutex lock;

    tbb::blocked_range<std::size_t> range{0, n, 1};

    const auto balance_delta = [&view](const auto num_nodes_source)
    {
        const std::int64_t difference =
            static_cast<std::int64_t>(view.NumberOfNodes()) / 2 - num_nodes_source;
        return std::abs(difference);
    };

    tbb::parallel_for(
        range,
        [&](const auto &chunk)
        {
            for (auto round = chunk.begin(), end = chunk.end(); round != end; ++round)
            {
                const auto slope = -1. + round * (2. / n);

                const auto terminals = makeTerminals(view, ratio, slope);
                auto cut = DinicMaxFlow()(view, terminals, cut_limit);
                if (!cut)
                    continue;
                auto cut_balance = get_balance(cut->num_nodes_source);

                {
                    std::lock_guard<std::mutex> guard{lock};

                    // Swap to keep the destruction of the old object outside of
                    // critical section.
                    if (cut->num_edges * cut_balance < best.num_edges * best_balance ||
                        (cut->num_edges == best.num_edges &&
                         balance_delta(cut->num_nodes_source) <
                             balance_delta(best.num_nodes_source)))
                    {
                        best_balance = cut_balance;
                        std::swap(best, *cut);
                        best_edge_count.store(best.num_edges, std::memory_order_relaxed);
                    }
                }
                // cut gets destroyed here
            }
        });

    return best;
}
} // namespace

DinicMaxFlow::MinCut computeInertialFlowCut(const BisectionGraphView &view,
                                            const std::size_t num_slopes,
                                            const double balance,
                                            const double source_sink_rate)
{ return bestMinCut(view, num_slopes, source_sink_rate, balance); }

} // namespace osrm::partitioner
