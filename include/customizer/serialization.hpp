#ifndef OSRM_CUSTOMIZER_SERIALIZATION_HPP
#define OSRM_CUSTOMIZER_SERIALIZATION_HPP

#include "partitioner/cell_storage.hpp"

#include "storage/serialization.hpp"
#include "storage/shared_memory_ownership.hpp"
#include "storage/tar.hpp"

namespace osrm
{
namespace customizer
{
namespace serialization
{

template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::CellMetricImpl<Ownership> &metric)
{
    storage::serialization::read(reader, name + "/weights", metric.weights);
    storage::serialization::read(reader, name + "/durations", metric.durations);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::CellMetricImpl<Ownership> &metric)
{
    storage::serialization::write(writer, name + "/weights", metric.weights);
    storage::serialization::write(writer, name + "/durations", metric.durations);
}
}
}
}

#endif
