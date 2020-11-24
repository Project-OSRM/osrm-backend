#ifndef OSRM_CONTRACTOR_SERIALIZATION_HPP
#define OSRM_CONTRACTOR_SERIALIZATION_HPP

#include "contractor/contracted_metric.hpp"

#include "util/serialization.hpp"

#include "storage/serialization.hpp"
#include "storage/tar.hpp"

namespace osrm
{
namespace contractor
{
namespace serialization
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
}
} // namespace serialization
} // namespace contractor
} // namespace osrm

#endif
