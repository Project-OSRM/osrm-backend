#ifndef OSMR_UTIL_SERIALIZATION_HPP
#define OSMR_UTIL_SERIALIZATION_HPP

#include "util/dynamic_graph.hpp"
#include "util/indexed_data.hpp"
#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"

#include "storage/io.hpp"
#include "storage/serialization.hpp"

namespace osrm::util::serialization
{

template <unsigned BlockSize, storage::Ownership Ownership>
void write(storage::tar::FileWriter &writer,
           const std::string &name,
           const util::RangeTable<BlockSize, Ownership> &table)
{
    writer.WriteFrom(name + "/sum_lengths.meta", table.sum_lengths);
    storage::serialization::write(writer, name + "/block_offsets", table.block_offsets);
    storage::serialization::write(writer, name + "/diff_blocks", table.diff_blocks);
}

template <unsigned BlockSize, storage::Ownership Ownership>
void read(storage::tar::FileReader &reader,
          const std::string &name,
          util::RangeTable<BlockSize, Ownership> &table)
{
    reader.ReadInto(name + "/sum_lengths.meta", table.sum_lengths);
    storage::serialization::read(reader, name + "/block_offsets", table.block_offsets);
    storage::serialization::read(reader, name + "/diff_blocks", table.diff_blocks);
}

template <typename T, std::size_t Bits, storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::PackedVector<T, Bits, Ownership> &vec)
{
    reader.ReadInto(name + "/number_of_elements.meta", vec.num_elements);
    storage::serialization::read(reader, name + "/packed", vec.vec);
}

template <typename T, std::size_t Bits, storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::PackedVector<T, Bits, Ownership> &vec)
{
    writer.WriteFrom(name + "/number_of_elements.meta", vec.num_elements);
    storage::serialization::write(writer, name + "/packed", vec.vec);
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 StaticGraph<EdgeDataT, Ownership> &graph)
{
    if (reader.HasEntry(name + "/node_blocks/headers"))
    {
        if constexpr (Ownership != storage::Ownership::View)
        {
            std::vector<util::BlockDeltaHeader> headers;
            std::vector<std::uint64_t> words;
            storage::serialization::read(reader, name + "/node_blocks/headers", headers);
            storage::serialization::read(reader, name + "/node_blocks/words", words);
            std::uint64_t count;
            reader.ReadInto(name + "/node_blocks/count", count);
            graph.node_array.resize(count);
            for (std::size_t i = 0; i < count; ++i)
                graph.node_array[i].first_edge = util::decodeBlockDelta(
                    {headers.data(), headers.size()}, {words.data(), words.size()}, i);
        }
        else
        {
            if (!graph.node_count)
                throw util::exception(
                    "Graph view storage is not bound for compressed node offsets");
            storage::serialization::read(reader, name + "/node_blocks/headers", graph.node_headers);
            storage::serialization::read(reader, name + "/node_blocks/words", graph.node_words);
            reader.ReadInto(name + "/node_blocks/count", *graph.node_count);
        }
    }
    else
        storage::serialization::read(reader, name + "/node_array", graph.node_array);
    if (reader.HasEntry(name + "/edge_array6"))
    {
        if constexpr (Ownership != storage::Ownership::View ||
                      !requires { EdgeDataT::SupportsPackedRecords; })
            throw util::exception("Packed query edges cannot be preprocessed");
        else
            storage::serialization::read(reader, name + "/edge_array6", graph.packed_edges);
    }
    else
        storage::serialization::read(reader, name + "/edge_array", graph.edge_array);
    graph.number_of_nodes = (graph.node_count ? *graph.node_count : graph.node_array.size()) - 1;
    graph.number_of_edges =
        graph.packed_edges.empty() ? graph.edge_array.size() : (graph.packed_edges.size() - 8) / 6;
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const StaticGraph<EdgeDataT, Ownership> &graph)
{
    storage::serialization::write(writer, name + "/node_array", graph.node_array);
    storage::serialization::write(writer, name + "/edge_array", graph.edge_array);
}

template <typename EdgeDataT>
inline void
read(storage::tar::FileReader &reader, const std::string &name, DynamicGraph<EdgeDataT> &graph)
{
    storage::serialization::read(reader, name + "/node_array", graph.node_array);
    const auto num_edges = reader.ReadElementCount64(name + "/edge_list");
    graph.edge_list.resize(num_edges);
    reader.ReadStreaming<typename std::remove_reference_t<decltype(graph)>::Edge>(
        name + "/edge_list", graph.edge_list.begin());
    graph.number_of_nodes = graph.node_array.size();
    graph.number_of_edges = num_edges;
}

template <typename EdgeDataT>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const DynamicGraph<EdgeDataT> &graph)
{
    storage::serialization::write(writer, name + "/node_array", graph.node_array);
    writer.WriteElementCount64(name + "/edge_list", graph.number_of_edges);
    writer.WriteStreaming<typename std::remove_reference_t<decltype(graph)>::Edge>(
        name + "/edge_list", graph.edge_list.begin(), graph.number_of_edges);
}

template <typename BlockPolicy, storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::IndexedDataImpl<BlockPolicy, Ownership> &index_data)
{
    storage::serialization::read(reader, name + "/blocks", index_data.blocks);
    storage::serialization::read(reader, name + "/values", index_data.values);
}

template <typename BlockPolicy, storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::IndexedDataImpl<BlockPolicy, Ownership> &index_data)
{
    storage::serialization::write(writer, name + "/blocks", index_data.blocks);
    storage::serialization::write(writer, name + "/values", index_data.values);
}

template <class EdgeDataT,
          storage::Ownership Ownership,
          std::uint32_t BRANCHING_FACTOR,
          std::uint32_t LEAF_PAGE_SIZE>
void read(storage::tar::FileReader &reader,
          const std::string &name,
          util::StaticRTree<EdgeDataT, Ownership, BRANCHING_FACTOR, LEAF_PAGE_SIZE> &rtree)
{
    storage::serialization::read(reader, name + "/search_tree", rtree.m_search_tree);
    storage::serialization::read(
        reader, name + "/search_tree_level_starts", rtree.m_tree_level_starts);
}

template <class EdgeDataT,
          storage::Ownership Ownership,
          std::uint32_t BRANCHING_FACTOR,
          std::uint32_t LEAF_PAGE_SIZE>
void write(storage::tar::FileWriter &writer,
           const std::string &name,
           const util::StaticRTree<EdgeDataT, Ownership, BRANCHING_FACTOR, LEAF_PAGE_SIZE> &rtree)
{
    storage::serialization::write(writer, name + "/search_tree", rtree.m_search_tree);
    storage::serialization::write(
        writer, name + "/search_tree_level_starts", rtree.m_tree_level_starts);
}
} // namespace osrm::util::serialization

#endif
