#ifndef OSRM_CONTRACTOR_CONTRACTED_EDGE_CONTAINER_HPP
#define OSRM_CONTRACTOR_CONTRACTED_EDGE_CONTAINER_HPP

#include "contractor/query_edge.hpp"

#include "util/integer_range.hpp"
#include "util/permutation.hpp"

#include <tbb/parallel_sort.h>

#include <algorithm>
#include <climits>
#include <cstdint>
#include <numeric>
#include <vector>

namespace osrm::contractor
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
    void Insert(std::vector<QueryEdge> new_edges)
    {
        BOOST_ASSERT(edges.size() == 0);
        BOOST_ASSERT(flags.empty());

        edges = std::move(new_edges);
        flags.resize(edges.size(), ALL_FLAGS);
    }

    void Filter(const std::vector<bool> &filter, std::size_t index)
    {
        BOOST_ASSERT(index < sizeof(MergedFlags) * CHAR_BIT);
        const MergedFlags flag = 1 << index;

        for (auto edge_index : util::irange<std::size_t>(0, edges.size()))
        {
            auto allowed = filter[edges[edge_index].source] && filter[edges[edge_index].target];
            if (allowed)
            {
                flags[edge_index] |= flag;
            }
            else
            {
                flags[edge_index] &= ~flag;
            }
        }
    }

    void Merge(std::vector<QueryEdge> new_edges)
    {
        BOOST_ASSERT(index < sizeof(MergedFlags) * CHAR_BIT);

        const MergedFlags flag = 1 << index++;

        auto edge_iter = edges.cbegin();
        auto edge_end = edges.cend();
        auto flags_iter = flags.begin();

        // Remove all edges that are contained in the old set of edges and set the appropriate flag.
        auto new_end =
            std::remove_if(new_edges.begin(),
                           new_edges.end(),
                           [&](const QueryEdge &edge)
                           {
                               // check if the new edge would be sorted before the currend old edge
                               // if so it is not contained yet in the set of old edges
                               if (edge_iter == edge_end || mergeCompare(edge, *edge_iter))
                               {
                                   return false;
                               }

                               // find the first old edge that is equal or greater then the new edge
                               while (edge_iter != edge_end && mergeCompare(*edge_iter, edge))
                               {
                                   BOOST_ASSERT(flags_iter != flags.end());
                                   edge_iter++;
                                   flags_iter++;
                               }

                               // all new edges will be sorted after the old edges
                               if (edge_iter == edge_end)
                               {
                                   return false;
                               }

                               BOOST_ASSERT(edge_iter != edge_end);
                               if (mergable(edge, *edge_iter))
                               {
                                   *flags_iter = *flags_iter | flag;
                                   return true;
                               }
                               BOOST_ASSERT(mergeCompare(edge, *edge_iter));
                               return false;
                           });

        // append new edges
        edges.insert(edges.end(), new_edges.begin(), new_end);
        auto edges_size = edges.size();
        auto new_edges_size = std::distance(new_edges.begin(), new_end);
        BOOST_ASSERT(static_cast<int>(edges_size) >= new_edges_size);
        flags.resize(edges_size);
        std::fill(flags.begin() + edges_size - new_edges_size, flags.end(), flag);

        // enforce sorting for next merge step
        std::vector<unsigned> ordering(edges_size);
        std::iota(ordering.begin(), ordering.end(), 0);
        tbb::parallel_sort(ordering.begin(),
                           ordering.end(),
                           [&](const auto lhs_idx, const auto rhs_idx)
                           { return mergeCompare(edges[lhs_idx], edges[rhs_idx]); });
        auto permutation = util::orderingToPermutation(ordering);

        util::inplacePermutation(edges.begin(), edges.end(), permutation);
        util::inplacePermutation(flags.begin(), flags.end(), permutation);
        BOOST_ASSERT(std::is_sorted(edges.begin(), edges.end(), mergeCompare));
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
    std::vector<QueryEdge> edges;
};
} // namespace osrm::contractor

#endif
