#ifndef OSRM_PARTITIONER_CUSTOMIZE_CELL_STORAGE_HPP
#define OSRM_PARTITIONER_CUSTOMIZE_CELL_STORAGE_HPP

#include "partitioner/multi_level_partition.hpp"

#include "util/assert.hpp"
#include "util/for_each_range.hpp"
#include "util/log.hpp"
#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

#include "storage/io_fwd.hpp"
#include "storage/shared_memory_ownership.hpp"

#include "customizer/cell_metric.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

#include <tbb/parallel_sort.h>

#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

namespace osrm
{
namespace partitioner
{
namespace detail
{
template <storage::Ownership Ownership> class CellStorageImpl;
}
using CellStorage = detail::CellStorageImpl<storage::Ownership::Container>;
using CellStorageView = detail::CellStorageImpl<storage::Ownership::View>;

namespace serialization
{
template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::CellStorageImpl<Ownership> &storage);
template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::CellStorageImpl<Ownership> &storage);
}

namespace detail
{
template <storage::Ownership Ownership> class CellStorageImpl
{
  public:
    using ValueOffset = std::uint32_t;
    using BoundaryOffset = std::uint32_t;
    using BoundarySize = std::uint32_t;
    using SourceIndex = std::uint32_t;
    using DestinationIndex = std::uint32_t;

    static constexpr auto INVALID_VALUE_OFFSET = std::numeric_limits<ValueOffset>::max();
    static constexpr auto INVALID_BOUNDARY_OFFSET = std::numeric_limits<BoundaryOffset>::max();

    struct CellData
    {
        ValueOffset value_offset = INVALID_VALUE_OFFSET;
        BoundaryOffset source_boundary_offset = INVALID_BOUNDARY_OFFSET;
        BoundaryOffset destination_boundary_offset = INVALID_BOUNDARY_OFFSET;
        BoundarySize num_source_nodes = 0;
        BoundarySize num_destination_nodes = 0;
    };

  private:
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;

    // Implementation of the cell view. We need a template parameter here
    // because we need to derive a read-only and read-write view from this.
    template <typename WeightValueT, typename DurationValueT, typename DistanceValueT>
    class CellImpl
    {
      private:
        using WeightPtrT = WeightValueT *;
        using DurationPtrT = DurationValueT *;
        using DistancePtrT = DistanceValueT *;
        BoundarySize num_source_nodes;
        BoundarySize num_destination_nodes;

        WeightPtrT const weights;
        DurationPtrT const durations;
        DistancePtrT const distances;
        const NodeID *const source_boundary;
        const NodeID *const destination_boundary;

        using RowIterator = WeightPtrT;
        // Possibly replace with
        // http://www.boost.org/doc/libs/1_55_0/libs/range/doc/html/range/reference/adaptors/reference/strided.html

        template <typename ValuePtrT>
        class ColumnIterator : public boost::iterator_facade<ColumnIterator<ValuePtrT>,
                                                             decltype(*std::declval<ValuePtrT>()),
                                                             boost::random_access_traversal_tag>
        {

            using ValueT = decltype(*std::declval<ValuePtrT>());
            typedef boost::iterator_facade<ColumnIterator<ValueT>,
                                           ValueT,
                                           boost::random_access_traversal_tag>
                base_t;

          public:
            typedef typename base_t::value_type value_type;
            typedef typename base_t::difference_type difference_type;
            typedef typename base_t::reference reference;
            typedef std::random_access_iterator_tag iterator_category;

            explicit ColumnIterator() : current(nullptr), stride(1) {}

            explicit ColumnIterator(ValuePtrT begin, std::size_t row_length)
                : current(begin), stride(row_length)
            {
                BOOST_ASSERT(begin != nullptr);
            }

          private:
            void increment() { current += stride; }
            void decrement() { current -= stride; }
            void advance(difference_type offset) { current += stride * offset; }
            bool equal(const ColumnIterator &other) const { return current == other.current; }
            reference dereference() const { return *current; }
            difference_type distance_to(const ColumnIterator &other) const
            {
                return (other.current - current) / static_cast<std::intptr_t>(stride);
            }

            friend class ::boost::iterator_core_access;
            ValuePtrT current;
            const std::size_t stride;
        };

        template <typename ValuePtr> auto GetOutRange(const ValuePtr ptr, const NodeID node) const
        {
            auto iter = std::find(source_boundary, source_boundary + num_source_nodes, node);
            if (iter == source_boundary + num_source_nodes)
                return boost::make_iterator_range(ptr, ptr);

            auto row = std::distance(source_boundary, iter);
            auto begin = ptr + num_destination_nodes * row;
            auto end = begin + num_destination_nodes;
            return boost::make_iterator_range(begin, end);
        }

        template <typename ValuePtr> auto GetInRange(const ValuePtr ptr, const NodeID node) const
        {
            auto iter =
                std::find(destination_boundary, destination_boundary + num_destination_nodes, node);
            if (iter == destination_boundary + num_destination_nodes)
                return boost::make_iterator_range(ColumnIterator<ValuePtr>{},
                                                  ColumnIterator<ValuePtr>{});

            auto column = std::distance(destination_boundary, iter);
            auto begin = ColumnIterator<ValuePtr>{ptr + column, num_destination_nodes};
            auto end = ColumnIterator<ValuePtr>{
                ptr + column + num_source_nodes * num_destination_nodes, num_destination_nodes};
            return boost::make_iterator_range(begin, end);
        }

      public:
        auto GetOutWeight(NodeID node) const { return GetOutRange(weights, node); }

        auto GetInWeight(NodeID node) const { return GetInRange(weights, node); }

        auto GetOutDuration(NodeID node) const { return GetOutRange(durations, node); }

        auto GetInDuration(NodeID node) const { return GetInRange(durations, node); }

        auto GetInDistance(NodeID node) const { return GetInRange(distances, node); }

        auto GetOutDistance(NodeID node) const { return GetOutRange(distances, node); }

        auto GetSourceNodes() const
        {
            return boost::make_iterator_range(source_boundary, source_boundary + num_source_nodes);
        }

        auto GetDestinationNodes() const
        {
            return boost::make_iterator_range(destination_boundary,
                                              destination_boundary + num_destination_nodes);
        }

        CellImpl(const CellData &data,
                 WeightPtrT const all_weights,
                 DurationPtrT const all_durations,
                 DistancePtrT const all_distances,
                 const NodeID *const all_sources,
                 const NodeID *const all_destinations)
            : num_source_nodes{data.num_source_nodes},
              num_destination_nodes{data.num_destination_nodes},
              weights{all_weights + data.value_offset},
              durations{all_durations + data.value_offset},
              distances{all_distances + data.value_offset},
              source_boundary{all_sources + data.source_boundary_offset},
              destination_boundary{all_destinations + data.destination_boundary_offset}
        {
            BOOST_ASSERT(all_weights != nullptr);
            BOOST_ASSERT(all_durations != nullptr);
            BOOST_ASSERT(all_distances != nullptr);
            BOOST_ASSERT(num_source_nodes == 0 || all_sources != nullptr);
            BOOST_ASSERT(num_destination_nodes == 0 || all_destinations != nullptr);
        }

        // Consturcts an emptry cell without weights. Useful when only access
        // to the cell structure is needed, without a concrete metric.
        CellImpl(const CellData &data,
                 const NodeID *const all_sources,
                 const NodeID *const all_destinations)
            : num_source_nodes{data.num_source_nodes},
              num_destination_nodes{data.num_destination_nodes}, weights{nullptr},
              durations{nullptr}, distances{nullptr},
              source_boundary{all_sources + data.source_boundary_offset},
              destination_boundary{all_destinations + data.destination_boundary_offset}
        {
            BOOST_ASSERT(num_source_nodes == 0 || all_sources != nullptr);
            BOOST_ASSERT(num_destination_nodes == 0 || all_destinations != nullptr);
        }
    };

    std::size_t LevelIDToIndex(LevelID level) const { return level - 1; }

  public:
    using Cell = CellImpl<EdgeWeight, EdgeDuration, EdgeDistance>;
    using ConstCell = CellImpl<const EdgeWeight, const EdgeDuration, const EdgeDistance>;

    CellStorageImpl() {}

    template <typename GraphT,
              typename = std::enable_if<Ownership == storage::Ownership::Container>>
    CellStorageImpl(const partitioner::MultiLevelPartition &partition, const GraphT &base_graph)
    {
        // pre-allocate storge for CellData so we can have random access to it by cell id
        unsigned number_of_cells = 0;
        for (LevelID level = 1u; level < partition.GetNumberOfLevels(); ++level)
        {
            level_to_cell_offset.push_back(number_of_cells);
            number_of_cells += partition.GetNumberOfCells(level);
        }
        level_to_cell_offset.push_back(number_of_cells);
        cells.resize(number_of_cells);

        std::vector<std::pair<CellID, NodeID>> level_source_boundary;
        std::vector<std::pair<CellID, NodeID>> level_destination_boundary;

        std::size_t number_of_unconneced = 0;

        for (LevelID level = 1u; level < partition.GetNumberOfLevels(); ++level)
        {
            auto level_offset = level_to_cell_offset[LevelIDToIndex(level)];

            level_source_boundary.clear();
            level_destination_boundary.clear();

            for (auto node = 0u; node < base_graph.GetNumberOfNodes(); ++node)
            {
                const CellID cell_id = partition.GetCell(level, node);
                bool is_source_node = false;
                bool is_destination_node = false;
                bool is_boundary_node = false;

                for (auto edge : base_graph.GetAdjacentEdgeRange(node))
                {
                    auto other = base_graph.GetTarget(edge);
                    const auto &data = base_graph.GetEdgeData(edge);

                    is_boundary_node |= partition.GetCell(level, other) != cell_id;
                    is_source_node |= partition.GetCell(level, other) == cell_id && data.forward;
                    is_destination_node |=
                        partition.GetCell(level, other) == cell_id && data.backward;
                }

                if (is_boundary_node)
                {
                    if (is_source_node)
                        level_source_boundary.emplace_back(cell_id, node);
                    if (is_destination_node)
                        level_destination_boundary.emplace_back(cell_id, node);

                    // if a node is unconnected we still need to keep it for correctness
                    // this adds it to the destination array to form an "empty" column
                    if (!is_source_node && !is_destination_node)
                    {
                        number_of_unconneced++;
                        util::Log(logWARNING) << "Found unconnected boundary node " << node << "("
                                              << cell_id << ") on level " << (int)level;
                        level_destination_boundary.emplace_back(cell_id, node);
                    }
                }
            }

            tbb::parallel_sort(level_source_boundary.begin(), level_source_boundary.end());
            tbb::parallel_sort(level_destination_boundary.begin(),
                               level_destination_boundary.end());

            const auto insert_cell_boundary = [this, level_offset](auto &boundary,
                                                                   auto set_num_nodes_fn,
                                                                   auto set_boundary_offset_fn,
                                                                   auto begin,
                                                                   auto end) {
                BOOST_ASSERT(std::distance(begin, end) > 0);

                const auto cell_id = begin->first;
                BOOST_ASSERT(level_offset + cell_id < cells.size());
                auto &cell = cells[level_offset + cell_id];
                set_num_nodes_fn(cell, std::distance(begin, end));
                set_boundary_offset_fn(cell, boundary.size());

                std::transform(begin,
                               end,
                               std::back_inserter(boundary),
                               [](const auto &cell_and_node) { return cell_and_node.second; });
            };

            util::for_each_range(
                level_source_boundary.begin(),
                level_source_boundary.end(),
                [this, insert_cell_boundary](auto begin, auto end) {
                    insert_cell_boundary(
                        source_boundary,
                        [](auto &cell, auto value) { cell.num_source_nodes = value; },
                        [](auto &cell, auto value) { cell.source_boundary_offset = value; },
                        begin,
                        end);
                });
            util::for_each_range(
                level_destination_boundary.begin(),
                level_destination_boundary.end(),
                [this, insert_cell_boundary](auto begin, auto end) {
                    insert_cell_boundary(
                        destination_boundary,
                        [](auto &cell, auto value) { cell.num_destination_nodes = value; },
                        [](auto &cell, auto value) { cell.destination_boundary_offset = value; },
                        begin,
                        end);
                });
        }

        // a partition that contains boundary nodes that have no arcs going into
        // the cells or coming out of it is bad. These nodes should be reassigned
        // to a different cell.
        if (number_of_unconneced > 0)
        {
            util::Log(logWARNING) << "Node needs to either have incoming or outgoing edges in cell."
                                  << " Number of unconnected nodes is " << number_of_unconneced;
        }

        // Set cell values offsets and calculate total storage size
        ValueOffset value_offset = 0;
        for (auto &cell : cells)
        {
            cell.value_offset = value_offset;
            value_offset += cell.num_source_nodes * cell.num_destination_nodes;
        }
    }

    // Returns a new metric that can be used with this container
    customizer::CellMetric MakeMetric() const
    {
        customizer::CellMetric metric;

        if (cells.empty())
        {
            return metric;
        }

        const auto &last_cell = cells.back();
        ValueOffset total_size =
            last_cell.value_offset + last_cell.num_source_nodes * last_cell.num_destination_nodes;

        metric.weights.resize(total_size + 1, INVALID_EDGE_WEIGHT);
        metric.durations.resize(total_size + 1, MAXIMAL_EDGE_DURATION);
        metric.distances.resize(total_size + 1, INVALID_EDGE_DISTANCE);

        return metric;
    }

    template <typename = std::enable_if<Ownership == storage::Ownership::View>>
    CellStorageImpl(Vector<NodeID> source_boundary_,
                    Vector<NodeID> destination_boundary_,
                    Vector<CellData> cells_,
                    Vector<std::uint64_t> level_to_cell_offset_)
        : source_boundary(std::move(source_boundary_)),
          destination_boundary(std::move(destination_boundary_)), cells(std::move(cells_)),
          level_to_cell_offset(std::move(level_to_cell_offset_))
    {
    }

    ConstCell GetCell(const customizer::detail::CellMetricImpl<Ownership> &metric,
                      LevelID level,
                      CellID id) const
    {
        const auto level_index = LevelIDToIndex(level);
        BOOST_ASSERT(level_index < level_to_cell_offset.size());
        const auto offset = level_to_cell_offset[level_index];
        const auto cell_index = offset + id;
        BOOST_ASSERT(cell_index < cells.size());
        return ConstCell{cells[cell_index],
                         metric.weights.data(),
                         metric.durations.data(),
                         metric.distances.data(),
                         source_boundary.empty() ? nullptr : source_boundary.data(),
                         destination_boundary.empty() ? nullptr : destination_boundary.data()};
    }

    ConstCell GetUnfilledCell(LevelID level, CellID id) const
    {
        const auto level_index = LevelIDToIndex(level);
        BOOST_ASSERT(level_index < level_to_cell_offset.size());
        const auto offset = level_to_cell_offset[level_index];
        const auto cell_index = offset + id;
        BOOST_ASSERT(cell_index < cells.size());
        return ConstCell{cells[cell_index],
                         source_boundary.empty() ? nullptr : source_boundary.data(),
                         destination_boundary.empty() ? nullptr : destination_boundary.data()};
    }

    template <typename = std::enable_if<Ownership == storage::Ownership::Container>>
    Cell GetCell(customizer::CellMetric &metric, LevelID level, CellID id) const
    {
        const auto level_index = LevelIDToIndex(level);
        BOOST_ASSERT(level_index < level_to_cell_offset.size());
        const auto offset = level_to_cell_offset[level_index];
        const auto cell_index = offset + id;
        BOOST_ASSERT(cell_index < cells.size());
        return Cell{cells[cell_index],
                    metric.weights.data(),
                    metric.durations.data(),
                    metric.distances.data(),
                    source_boundary.data(),
                    destination_boundary.data()};
    }

    friend void serialization::read<Ownership>(storage::tar::FileReader &reader,
                                               const std::string &name,
                                               detail::CellStorageImpl<Ownership> &storage);
    friend void serialization::write<Ownership>(storage::tar::FileWriter &writer,
                                                const std::string &name,
                                                const detail::CellStorageImpl<Ownership> &storage);

  private:
    Vector<NodeID> source_boundary;
    Vector<NodeID> destination_boundary;
    Vector<CellData> cells;
    Vector<std::uint64_t> level_to_cell_offset;
};
}
}
}

#endif // OSRM_PARTITIONER_CUSTOMIZE_CELL_STORAGE_HPP
