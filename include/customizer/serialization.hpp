#ifndef OSRM_CUSTOMIZER_SERIALIZATION_HPP
#define OSRM_CUSTOMIZER_SERIALIZATION_HPP

#include "partition/cell_storage.hpp"

#include "storage/io.hpp"
#include "storage/serialization.hpp"
#include "storage/shared_memory_ownership.hpp"

namespace osrm
{
namespace customizer
{
namespace serialization
{

template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader, detail::CellMetricImpl<Ownership> &metric)
{
    storage::serialization::read(reader, metric.weights);
    storage::serialization::read(reader, metric.durations);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer, const detail::CellMetricImpl<Ownership> &metric)
{
    storage::serialization::write(writer, metric.weights);
    storage::serialization::write(writer, metric.durations);
}
}
}
}

#endif
