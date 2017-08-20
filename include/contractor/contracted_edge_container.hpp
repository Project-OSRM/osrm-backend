#ifndef OSRM_CONTRACTOR_CONTRACTED_EDGE_CONTAINER_HPP
#define OSRM_CONTRACTOR_CONTRACTED_EDGE_CONTAINER_HPP

#include "contractor/query_edge.hpp"
#include "util/deallocating_vector.hpp"

#include <climits>

namespace osrm
{
namespace contractor
{

struct ContractedEdgeContainer
{
  private:
    using MergedFlags = std::uint8_t;
    static constexpr auto ALL_FLAGS = 0xFF;

    static bool mergeCompare(const QueryEdge &lhs, const QueryEdge &rhs)
    {
        return std::tie(lhs.source,
                        lhs.target,
                        lhs.data.shortcut,
                        lhs.data.turn_id,
                        lhs.data.weight,
                        lhs.data.duration,
                        lhs.data.forward,
                        lhs.data.backward) < std::tie(rhs.source,
                                                      rhs.target,
                                                      rhs.data.shortcut,
                                                      rhs.data.turn_id,
                                                      rhs.data.weight,
                                                      rhs.data.duration,
                                                      rhs.data.forward,
                                                      rhs.data.backward);
    }

    static bool mergable(const QueryEdge &lhs, const QueryEdge &rhs)
    {
        // only true if both are equal
        return !mergeCompare(lhs, rhs) && !mergeCompare(rhs, lhs);
    }

  public:
    void Insert(util::DeallocatingVector<QueryEdge> new_edges)
    {
        BOOST_ASSERT(edges.size() == 0);
        BOOST_ASSERT(flags.empty());

        edges = std::move(new_edges);
        flags.resize(edges.size(), ALL_FLAGS);
    }

    void Merge(util::DeallocatingVector<QueryEdge> new_edges)
    {
        BOOST_ASSERT(index < sizeof(MergedFlags) * CHAR_BIT);

        const MergedFlags flag = 1 << index++;

        std::vector<MergedFlags> merged_flags;
        merged_flags.reserve(flags.size() * 1.1);
        util::DeallocatingVector<QueryEdge> merged_edges;
        merged_edges.reserve(edges.size() * 1.1);

        auto flags_iter = flags.begin();
        // destructive iterators, this is single-pass only
        // FIXME using dbegin() dend() will result in segfaults.
        auto edges_iter = edges.dbegin();
        auto edges_end = edges.dend();
        auto new_edges_iter = new_edges.dbegin();
        auto new_edges_end = new_edges.dend();

        while (edges_iter != edges_end && new_edges_iter != new_edges_end)
        {
            while (edges_iter != edges_end && mergeCompare(*edges_iter, *new_edges_iter))
            {
                merged_edges.push_back(*edges_iter);
                merged_flags.push_back(*flags_iter);
                edges_iter++;
                flags_iter++;
            }

            if (edges_iter == edges_end)
            {
                break;
            }

            while (new_edges_iter != new_edges_end && mergeCompare(*new_edges_iter, *edges_iter))
            {
                merged_edges.push_back(*new_edges_iter);
                merged_flags.push_back(flag);
                new_edges_iter++;
            }

            if (new_edges_iter == new_edges_end)
            {
                break;
            }

            while (edges_iter != edges_end && new_edges_iter != new_edges_end &&
                   mergable(*edges_iter, *new_edges_iter))
            {
                merged_edges.push_back(*edges_iter);
                merged_flags.push_back(*flags_iter | flag);

                edges_iter++;
                flags_iter++;
                new_edges_iter++;
            }
        }

        while (edges_iter != edges_end)
        {
            BOOST_ASSERT(new_edges_iter == new_edges_end);
            merged_edges.push_back(*edges_iter++);
            merged_flags.push_back(*flags_iter++);
        }

        while (new_edges_iter != new_edges_end)
        {
            BOOST_ASSERT(edges_iter == edges_end);
            merged_edges.push_back(*new_edges_iter++);
            merged_flags.push_back(flag);
        }

        flags = std::move(merged_flags);
        edges = std::move(merged_edges);
    }

    auto MakeEdgeFilters() const
    {
        std::vector<std::vector<bool>> filters(index);
        for (const auto flag_index : util::irange<std::size_t>(0, index))
        {
            MergedFlags mask = 1 << flag_index;
            for (const auto flag : flags)
            {
                filters[flag_index].push_back(flag & mask);
            }
        }

        return filters;
    }

    std::size_t index = 0;
    std::vector<MergedFlags> flags;
    util::DeallocatingVector<QueryEdge> edges;
};
}
}

#endif
