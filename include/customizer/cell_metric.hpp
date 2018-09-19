#ifndef OSRM_CUSTOMIZER_CELL_METRIC_HPP
#define OSRM_CUSTOMIZER_CELL_METRIC_HPP

#include "storage/io_fwd.hpp"
#include "storage/shared_memory_ownership.hpp"

#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

namespace osrm
{
namespace customizer
{
namespace detail
{
// Encapsulated one metric to make it easily replacable in CelLStorage
template <storage::Ownership Ownership> struct CellMetricImpl
{
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;

    Vector<EdgeWeight> weights;
    Vector<EdgeDuration> durations;
    Vector<EdgeDistance> distances;
};
}

using CellMetric = detail::CellMetricImpl<storage::Ownership::Container>;
using CellMetricView = detail::CellMetricImpl<storage::Ownership::View>;
}
}

#endif
