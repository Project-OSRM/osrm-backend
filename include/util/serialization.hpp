#ifndef OSMR_UTIL_SERIALIZATION_HPP
#define OSMR_UTIL_SERIALIZATION_HPP

#include "util/static_graph.hpp"
#include "util/dynamic_graph.hpp"
#include "storage/io.hpp"

namespace osrm
{
namespace util
{
namespace serialization
{

template <typename EdgeDataT, storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 StaticGraph<EdgeDataT, Ownership> &graph)
{
    reader.DeserializeVector(graph.node_array);
    reader.DeserializeVector(graph.edge_array);
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                 const StaticGraph<EdgeDataT, Ownership> &graph)
{
    writer.SerializeVector(graph.node_array);
    writer.SerializeVector(graph.edge_array);
}

template <typename EdgeDataT>
inline void read(storage::io::FileReader &reader,
                 DynamicGraph<EdgeDataT> &graph)
{
    reader.DeserializeVector(graph.node_array);
    auto num_edges = reader.ReadElementCount64();
    graph.edge_list.resize(num_edges);
    for (auto index : irange<std::size_t>(0, num_edges))
    {
        reader.ReadOne(graph.edge_list[index]);
    }
    graph.number_of_nodes = graph.node_array.size();
    graph.number_of_edges = num_edges;
}

template <typename EdgeDataT>
inline void write(storage::io::FileWriter &writer,
                 const DynamicGraph<EdgeDataT> &graph)
{
    writer.SerializeVector(graph.node_array);
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
