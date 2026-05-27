#include "util/format.hpp"
#include "util/linear_hash_storage.hpp"
#include "util/log.hpp"
#include "util/query_heap.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

using namespace osrm;
using namespace std;

#ifdef _WIN32
#pragma optimize("", off)
template <class T> void dont_optimize_away(T &&datum) { T local = datum; }
#pragma optimize("", on)
#else
template <class T> void dont_optimize_away(T &&datum) { asm volatile("" : "+r"(datum)); }
#endif

/**
 * The size of the hash table. No more than `size` -1 entries can be stored in the
 * hash table.
 */
size_t storage_size = 1 << 16;
const int num_rounds = 100;

template <class StorageT> auto bench(const size_t num_entries, const size_t num_nodes)
{
    vector<size_t> nodes(num_entries);
    mt19937 g(1337);
    // iota(indices.begin(), indices.end(), 0);
    // shuffle(indices.begin(), indices.end(), g);

    uniform_int_distribution<size_t> distrib(0, num_nodes - 1);
    distrib(g);
    generate(nodes.begin(), nodes.end(), [&]() { return distrib(g); });

    StorageT storage(storage_size);

    TIMER_START(write);
    for (size_t i = 0; i < num_rounds; ++i)
    {
        for (size_t j = 0; j < num_entries; ++j)
        {
            storage[nodes[j]] = j;
        }
    }
    TIMER_STOP(write);

    auto sum = 0;
    TIMER_START(read);
    for (size_t i = 0; i < num_rounds; ++i)
    {
        for (size_t j = 0; j < num_entries; ++j)
        {
            sum += storage.peek_index(nodes[j]);
        }
    }
    TIMER_STOP(read);
    dont_optimize_away(sum);

    return osrm::util::compat::format(
        "{:9.1f} | {:10.1f} |", (0.001 * TIMER_USEC(read)), (0.001 * TIMER_USEC(write)));
}

int main(int, char **)
{
    util::LogPolicy::GetInstance().Unmute();

    /**
     * Occupancy of LinearHash in percent. Performance drops drastically with
     * high occupancies.
     */
    vector<int> occupancies{50, 75, 90, 95, 99};
    /**
     * The number of nodes extracted from OSM. The contractor works with internal node
     * ids, not OSM node ids. Internal node ids are 32 bit long and consecutive starting
     * with 0. To simulate realistic conditions we assume the extractor has generated
     * `num_nodes` nodes and then test the hasher on a random subset of these nodes.
     * Set to half the max. possible nodes.
     */
    size_t num_nodes = 1u << 31;

    for_each(occupancies.cbegin(),
             occupancies.cend(),
             [&](const int occupancy)
             {
                 int num_entries = occupancy * storage_size / 100;

                 cout << "number of nodes: " << num_nodes << " hash table size: " << storage_size
                      << " occupancy: " << occupancy << "%\n\n";

                 cout << "Storage      | read (ms) | write (ms) |" << endl;
                 cout << "-------------|----------:|-----------:|" << endl;
                 cout << "UnorderedMap | "
                      << bench<util::UnorderedMapStorage<NodeID, NodeID>>(num_entries, num_nodes)
                      << endl;
                 cout << "LinearHash   | "
                      << bench<util::LinearHashStorage<NodeID, NodeID>>(num_entries, num_nodes)
                      << endl;
                 cout << endl;
             });

    cout << "Done\n";
}
