#ifndef OSMR_UTIL_SERIALIZATION_HPP
#define OSMR_UTIL_SERIALIZATION_HPP

#include "util/dynamic_graph.hpp"
#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/static_graph.hpp"

#include "storage/io.hpp"
#include "storage/serialization.hpp"

namespace osrm
{
namespace util
{
namespace serialization
{
template <unsigned BlockSize, storage::Ownership Ownership>
void write(storage::io::FileWriter &writer, const util::RangeTable<BlockSize, Ownership> &table)
{
    writer.WriteOne(table.sum_lengths);
    storage::serialization::write(writer, table.block_offsets);
    storage::serialization::write(writer, table.diff_blocks);
}

template <unsigned BlockSize, storage::Ownership Ownership>
void read(storage::io::FileReader &reader, util::RangeTable<BlockSize, Ownership> &table)
{
    table.sum_lengths = reader.ReadOne<unsigned>();
    storage::serialization::read(reader, table.block_offsets);
    storage::serialization::read(reader, table.diff_blocks);
}

template <typename T, std::size_t Bits, storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader, detail::PackedVector<T, Bits, Ownership> &vec)
{
    vec.num_elements = reader.ReadOne<std::uint64_t>();
    storage::serialization::read(reader, vec.vec);
}

template <typename T, std::size_t Bits, storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::PackedVector<T, Bits, Ownership> &vec)
{
    writer.WriteOne(vec.num_elements);
    storage::serialization::write(writer, vec.vec);
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader, StaticGraph<EdgeDataT, Ownership> &graph)
{
    storage::serialization::read(reader, graph.node_array);
    storage::serialization::read(reader, graph.edge_array);
    graph.number_of_nodes = graph.node_array.size() - 1;
    graph.number_of_edges = graph.edge_array.size();
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer, const StaticGraph<EdgeDataT, Ownership> &graph)
{
    storage::serialization::write(writer, graph.node_array);
    storage::serialization::write(writer, graph.edge_array);
}

template <typename EdgeDataT>
inline void read(storage::io::FileReader &reader, DynamicGraph<EdgeDataT> &graph)
{
    storage::serialization::read(reader, graph.node_array);
    const auto num_edges = reader.ReadElementCount64();
    graph.edge_list.resize(num_edges);
    for (auto index : irange<std::size_t>(0, num_edges))
    {
        reader.ReadOne(graph.edge_list[index]);
    }
    graph.number_of_nodes = graph.node_array.size();
    graph.number_of_edges = num_edges;
}

template <typename EdgeDataT>
inline void write(storage::io::FileWriter &writer, const DynamicGraph<EdgeDataT> &graph)
{
    storage::serialization::write(writer, graph.node_array);
    writer.WriteElementCount64(graph.number_of_edges);
    for (auto index : irange<std::size_t>(0, graph.number_of_edges))
    {
        writer.WriteOne(graph.edge_list[index]);
    }
}
}
}
}

#endif
