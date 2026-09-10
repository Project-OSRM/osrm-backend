#ifndef OSRM_CONTRACTOR_SERIALIZATION_HPP
#define OSRM_CONTRACTOR_SERIALIZATION_HPP

#include "contractor/contracted_metric.hpp"

#include "util/exception.hpp"
#include "util/serialization.hpp"

#include "storage/serialization.hpp"
#include "storage/tar.hpp"

#include <algorithm>
#include <array>

namespace osrm::contractor::serialization
{

template <storage::Ownership Ownership>
void write(storage::tar::FileWriter &writer,
           const std::string &name,
           const detail::ContractedMetric<Ownership> &metric)
{
    util::serialization::write(writer, name + "/contracted_graph", metric.graph);

    writer.WriteElementCount64(name + "/exclude", metric.edge_filter.size());
    for (const auto index : util::irange<std::size_t>(0, metric.edge_filter.size()))
    {
        storage::serialization::write(writer,
                                      name + "/exclude/" + std::to_string(index) + "/edge_filter",
                                      metric.edge_filter[index]);
    }

    if (!metric.isochrone_graph.empty())
    {
        if (!engine::isochrone::isValidDurationGraph(metric.isochrone_graph,
                                                     metric.graph.GetNumberOfNodes()))
        {
            throw util::exception("Invalid isochrone duration graph in contracted metric");
        }

        storage::serialization::write(
            writer, name + "/isochrone/forward_offsets", metric.isochrone_graph.forward_offsets);
        storage::serialization::write(
            writer, name + "/isochrone/forward_arcs", metric.isochrone_graph.forward_arcs);
        storage::serialization::write(
            writer, name + "/isochrone/reverse_offsets", metric.isochrone_graph.reverse_offsets);
        storage::serialization::write(
            writer, name + "/isochrone/reverse_arcs", metric.isochrone_graph.reverse_arcs);
    }
}

template <storage::Ownership Ownership>
void read(storage::tar::FileReader &reader,
          const std::string &name,
          detail::ContractedMetric<Ownership> &metric)
{
    util::serialization::read(reader, name + "/contracted_graph", metric.graph);

    metric.edge_filter.resize(reader.ReadElementCount64(name + "/exclude"));
    for (const auto index : util::irange<std::size_t>(0, metric.edge_filter.size()))
    {
        storage::serialization::read(reader,
                                     name + "/exclude/" + std::to_string(index) + "/edge_filter",
                                     metric.edge_filter[index]);
    }

    const std::array names = {name + "/isochrone/forward_offsets",
                              name + "/isochrone/forward_arcs",
                              name + "/isochrone/reverse_offsets",
                              name + "/isochrone/reverse_arcs"};
    const auto has_vector = [&reader](const std::string &vector_name)
    { return reader.HasEntry(vector_name) && reader.HasEntry(vector_name + ".meta"); };
    const auto has_any_entry = [&reader](const std::string &vector_name)
    { return reader.HasEntry(vector_name) || reader.HasEntry(vector_name + ".meta"); };
    const auto present = std::count_if(names.begin(), names.end(), has_vector);
    const auto any_present = std::any_of(names.begin(), names.end(), has_any_entry);

    if (!any_present)
    {
        metric.isochrone_graph = {};
        return;
    }

    if (present != names.size())
    {
        throw util::exception("Incomplete isochrone duration graph in contracted metric");
    }

    storage::serialization::read(reader, names[0], metric.isochrone_graph.forward_offsets);
    storage::serialization::read(reader, names[1], metric.isochrone_graph.forward_arcs);
    storage::serialization::read(reader, names[2], metric.isochrone_graph.reverse_offsets);
    storage::serialization::read(reader, names[3], metric.isochrone_graph.reverse_arcs);
    if (metric.isochrone_graph.empty() ||
        !engine::isochrone::isValidDurationGraph(metric.isochrone_graph,
                                                 metric.graph.GetNumberOfNodes()))
    {
        throw util::exception("Invalid isochrone duration graph in contracted metric");
    }
}
} // namespace osrm::contractor::serialization

#endif
