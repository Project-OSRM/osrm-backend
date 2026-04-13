#ifndef OSRM_CONTRACTOR_SEARCH_HPP
#define OSRM_CONTRACTOR_SEARCH_HPP

#include "contractor/contractor_graph.hpp"
#include "contractor/contractor_heap.hpp"

#include "util/typedefs.hpp"

#include <cstddef>

namespace osrm::contractor
{

/**
 * Stop search after this many settled nodes during contraction simulation.
 */
constexpr std::size_t SIMULATION_SEARCH_SPACE_SIZE = 1000;
/**
 * Stop search after this many settled nodes during contraction. Tuning parameter:
 * Bigger search space is slower but may produce smaller contracted graph.
 */
constexpr std::size_t FULL_SEARCH_SPACE_SIZE = 2000;
/**
 * Size of the contractor heap storage. Bigger heap storage is faster but increases
 * memory consumption. Must be a power of two.
 */
constexpr std::size_t HASH_MAP_CAPACITY = 1u << 13;

static_assert((HASH_MAP_CAPACITY & (HASH_MAP_CAPACITY - 1)) == 0,
              "HASH_MAP_CAPACITY must be a power of two");
/**
 * Stop search after this many relaxed nodes during contraction. Protection against heap
 * storage overflow. Must be less than HASH_MAP_CAPACITY. Should be 50-75% of HASH_MAP_CAPACITY.
 */
constexpr std::size_t RELAXED_NODE_LIMIT = HASH_MAP_CAPACITY * 0.75;

void search(ContractorHeap &heap,
            const ContractorGraph &graph,
            const std::vector<bool> &contractable,
            const unsigned number_of_targets,
            const int node_limit,
            const EdgeWeight weight_limit,
            const NodeID forbidden_node);

} // namespace osrm::contractor

#endif // OSRM_CONTRACTOR_DIJKSTRA_HPP
