#ifndef OSRM_CUSTOMIZER_SERIALIZATION_HPP
#define OSRM_CUSTOMIZER_SERIALIZATION_HPP

#include "customizer/edge_based_graph.hpp"

#include "partitioner/cell_storage.hpp"

#include "util/exception.hpp"

#include "storage/serialization.hpp"
#include "storage/shared_memory_ownership.hpp"
#include "storage/tar.hpp"

#include <algorithm>
#include <array>

namespace osrm::customizer::serialization
{

template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::CellMetricImpl<Ownership> &metric)
{
    storage::serialization::read(reader, name + "/weights", metric.weights);
    storage::serialization::read(reader, name + "/durations", metric.durations);
    storage::serialization::read(reader, name + "/distances", metric.distances);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::CellMetricImpl<Ownership> &metric)
{
    storage::serialization::write(writer, name + "/weights", metric.weights);
    storage::serialization::write(writer, name + "/durations", metric.durations);
    storage::serialization::write(writer, name + "/distances", metric.distances);
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    storage::serialization::read(reader, name + "/node_array", graph.node_array);
    storage::serialization::read(reader, name + "/node_weights", graph.node_weights);
    storage::serialization::read(reader, name + "/node_durations", graph.node_durations);
    storage::serialization::read(reader, name + "/node_distances", graph.node_distances);
    storage::serialization::read(reader, name + "/edge_array", graph.edge_array);
    storage::serialization::read(reader, name + "/is_forward_edge", graph.is_forward_edge);
    storage::serialization::read(reader, name + "/is_backward_edge", graph.is_backward_edge);
    storage::serialization::read(reader, name + "/node_to_edge_offset", graph.node_to_edge_offset);

    const std::array isochrone_names = {name + "/isochrone/forward_offsets",
                                        name + "/isochrone/forward_arcs",
                                        name + "/isochrone/reverse_offsets",
                                        name + "/isochrone/reverse_arcs"};
    const auto has_vector = [&reader](const std::string &vector_name)
    { return reader.HasEntry(vector_name) && reader.HasEntry(vector_name + ".meta"); };
    const auto has_any_entry = [&reader](const std::string &vector_name)
    { return reader.HasEntry(vector_name) || reader.HasEntry(vector_name + ".meta"); };
    const auto present = std::count_if(isochrone_names.begin(), isochrone_names.end(), has_vector);
    const auto any_present =
        std::any_of(isochrone_names.begin(), isochrone_names.end(), has_any_entry);

    if (!any_present)
    {
        graph.isochrone_graph = {};
        return;
    }

    if (present != isochrone_names.size())
        throw util::exception("Incomplete isochrone duration graph in MLD graph");

    storage::serialization::read(reader, isochrone_names[0], graph.isochrone_graph.forward_offsets);
    storage::serialization::read(reader, isochrone_names[1], graph.isochrone_graph.forward_arcs);
    storage::serialization::read(reader, isochrone_names[2], graph.isochrone_graph.reverse_offsets);
    storage::serialization::read(reader, isochrone_names[3], graph.isochrone_graph.reverse_arcs);
    if (graph.node_array.empty())
        throw util::exception("Invalid MLD graph without nodes");
    if (graph.isochrone_graph.empty() || !engine::isochrone::isValidDurationGraph(
                                             graph.isochrone_graph, graph.node_array.size() - 1))
        throw util::exception("Invalid isochrone duration graph in MLD graph");
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    storage::serialization::write(writer, name + "/node_array", graph.node_array);
    storage::serialization::write(writer, name + "/node_weights", graph.node_weights);
    storage::serialization::write(writer, name + "/node_durations", graph.node_durations);
    storage::serialization::write(writer, name + "/node_distances", graph.node_distances);
    storage::serialization::write(writer, name + "/edge_array", graph.edge_array);
    storage::serialization::write(writer, name + "/is_forward_edge", graph.is_forward_edge);
    storage::serialization::write(writer, name + "/is_backward_edge", graph.is_backward_edge);
    storage::serialization::write(writer, name + "/node_to_edge_offset", graph.node_to_edge_offset);

    if (!graph.isochrone_graph.empty())
    {
        if (graph.node_array.empty() || !engine::isochrone::isValidDurationGraph(
                                            graph.isochrone_graph, graph.node_array.size() - 1))
        {
            throw util::exception("Invalid isochrone duration graph in MLD graph");
        }

        storage::serialization::write(
            writer, name + "/isochrone/forward_offsets", graph.isochrone_graph.forward_offsets);
        storage::serialization::write(
            writer, name + "/isochrone/forward_arcs", graph.isochrone_graph.forward_arcs);
        storage::serialization::write(
            writer, name + "/isochrone/reverse_offsets", graph.isochrone_graph.reverse_offsets);
        storage::serialization::write(
            writer, name + "/isochrone/reverse_arcs", graph.isochrone_graph.reverse_arcs);
    }
}
} // namespace osrm::customizer::serialization

#endif
