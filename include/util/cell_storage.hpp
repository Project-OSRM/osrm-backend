#ifndef OSRM_UTIL_CELL_STORAGE_HPP
#define OSRM_UTIL_CELL_STORAGE_HPP

#include "util/assert.hpp"
#include "util/for_each_range.hpp"
#include "util/log.hpp"
#include "util/multi_level_partition.hpp"
#include "util/shared_memory_vector_wrapper.hpp"
#include "util/typedefs.hpp"

#include "storage/io.hpp"

#include <boost/range/iterator_range.hpp>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

namespace osrm
{
namespace util
{

template <bool UseShareMemory> class CellStorage
{
  public:
    using WeightOffset = std::uint32_t;
    using BoundaryOffset = std::uint32_t;
    using BoundarySize = std::uint32_t;
    using SourceIndex = std::uint32_t;
    using DestinationIndex = std::uint32_t;

    static constexpr auto INVALID_WEIGHT_OFFSET = std::numeric_limits<WeightOffset>::max();
    static constexpr auto INVALID_BOUNDARY_OFFSET = std::numeric_limits<BoundaryOffset>::max();

  private:
    struct CellData
    {
        WeightOffset weight_offset = INVALID_WEIGHT_OFFSET;
        BoundaryOffset source_boundary_offset = INVALID_BOUNDARY_OFFSET;
        BoundaryOffset destination_boundary_offset = INVALID_BOUNDARY_OFFSET;
        BoundarySize num_source_nodes = 0;
        BoundarySize num_destination_nodes = 0;
    };

    // Implementation of the cell view. We need a template parameter here
    // because we need to derive a read-only and read-write view from this.
    template <typename WeightValueT> class CellImpl
    {
      private:
        using WeightPtrT = WeightValueT *;
        using WeightRefT = WeightValueT &;
        BoundarySize num_source_nodes;
        BoundarySize num_destination_nodes;

        WeightPtrT const weights;
        const NodeID *const source_boundary;
        const NodeID *const destination_boundary;

        using RowIterator = WeightPtrT;
        // Possibly replace with
        // http://www.boost.org/doc/libs/1_55_0/libs/range/doc/html/range/reference/adaptors/reference/strided.html
        class ColumnIterator : public std::iterator<std::random_access_iterator_tag, EdgeWeight>
        {
          public:
            explicit ColumnIterator(WeightPtrT begin, std::size_t row_length)
                : current(begin), stride(row_length)
            {
                BOOST_ASSERT(begin != nullptr);
            }

            WeightRefT operator*() const { return *current; }

            ColumnIterator &operator++()
            {
                current += stride;
                return *this;
            }

            ColumnIterator &operator+=(int amount)
            {
                current += stride * amount;
                return *this;
            }

            bool operator==(const ColumnIterator &other) const { return current == other.current; }

            bool operator!=(const ColumnIterator &other) const { return current != other.current; }

            std::int64_t operator-(const ColumnIterator &other) const
            {
                return (current - other.current) / stride;
            }

          private:
            WeightPtrT current;
            std::size_t stride;
        };

        std::size_t GetRow(NodeID node) const
        {
            auto iter = std::find(source_boundary, source_boundary + num_source_nodes, node);
            BOOST_ASSERT(iter != source_boundary + num_source_nodes);
            return iter - source_boundary;
        }

        std::size_t GetColumn(NodeID node) const
        {
            auto iter =
                std::find(destination_boundary, destination_boundary + num_destination_nodes, node);
            BOOST_ASSERT(iter != destination_boundary + num_destination_nodes);
            return iter - destination_boundary;
        }

      public:
        auto GetOutWeight(NodeID node) const
        {
            auto row = GetRow(node);
            auto begin = weights + num_destination_nodes * row;
            auto end = begin + num_destination_nodes;
            return boost::make_iterator_range(begin, end);
        }

        auto GetInWeight(NodeID node) const
        {
            auto column = GetColumn(node);
            auto begin = ColumnIterator{weights + column, num_destination_nodes};
            auto end = ColumnIterator{weights + column + num_source_nodes * num_destination_nodes,
                                      num_destination_nodes};
            return boost::make_iterator_range(begin, end);
        }

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
                 WeightPtrT const all_weight,
                 const NodeID *const all_sources,
                 const NodeID *const all_destinations)
            : num_source_nodes{data.num_source_nodes},
              num_destination_nodes{data.num_destination_nodes},
              weights{all_weight + data.weight_offset},
              source_boundary{all_sources + data.source_boundary_offset},
              destination_boundary{all_destinations + data.destination_boundary_offset}
        {
            BOOST_ASSERT(all_weight != nullptr);
            BOOST_ASSERT(all_sources != nullptr);
            BOOST_ASSERT(all_destinations != nullptr);
        }
    };

    std::size_t LevelIDToIndex(LevelID level) const { return level - 1; }

  public:
    using Cell = CellImpl<EdgeWeight>;
    using ConstCell = CellImpl<const EdgeWeight>;

    CellStorage() {}

    template <typename GraphT>
    CellStorage(const MultiLevelPartition &partition, const GraphT &base_graph)
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
                    // a partition that contains boundary nodes that have no arcs going into
                    // the cells or coming out of it is invalid. These nodes should be reassigned
                    // to a different cell.
                    BOOST_ASSERT_MSG(
                        is_source_node || is_destination_node,
                        "Node needs to either have incoming or outgoing edges in cell");
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

        // Set weight offsets and calculate total storage size
        WeightOffset weight_offset = 0;
        for (auto &cell : cells)
        {
            cell.weight_offset = weight_offset;
            weight_offset += cell.num_source_nodes * cell.num_destination_nodes;
        }

        weights.resize(weight_offset + 1, INVALID_EDGE_WEIGHT);
    }

    CellStorage(std::vector<EdgeWeight> weights_,
                std::vector<NodeID> source_boundary_,
                std::vector<NodeID> destination_boundary_,
                std::vector<CellData> cells_,
                std::vector<std::size_t> level_to_cell_offset_)
        : weights(std::move(weights_)), source_boundary(std::move(source_boundary_)),
          destination_boundary(std::move(destination_boundary_)), cells(std::move(cells_)),
          level_to_cell_offset(std::move(level_to_cell_offset_))
    {
    }

    CellStorage(const boost::filesystem::path &path) { Read(path); }

    ConstCell GetCell(LevelID level, CellID id) const
    {
        const auto level_index = LevelIDToIndex(level);
        BOOST_ASSERT(level_index < level_to_cell_offset.size());
        const auto offset = level_to_cell_offset[level_index];
        const auto cell_index = offset + id;
        BOOST_ASSERT(cell_index < cells.size());
        return ConstCell{
            cells[cell_index], weights.data(), source_boundary.data(), destination_boundary.data()};
    }

    Cell GetCell(LevelID level, CellID id)
    {
        const auto level_index = LevelIDToIndex(level);
        BOOST_ASSERT(level_index < level_to_cell_offset.size());
        const auto offset = level_to_cell_offset[level_index];
        const auto cell_index = offset + id;
        BOOST_ASSERT(cell_index < cells.size());
        return Cell{
            cells[cell_index], weights.data(), source_boundary.data(), destination_boundary.data()};
    }

    std::size_t GetRequiredMemorySize(const boost::filesystem::path &path) const
    {
        const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
        storage::io::FileReader reader{path, fingerprint};

        std::size_t memory_size = 0;
        memory_size += reader.GetVectorMemorySize<decltype(weights[0])>();
        memory_size += reader.GetVectorMemorySize<decltype(source_boundary[0])>();
        memory_size += reader.GetVectorMemorySize<decltype(destination_boundary[0])>();
        memory_size += reader.GetVectorMemorySize<decltype(cells[0])>();
        memory_size += reader.GetVectorMemorySize<decltype(level_to_cell_offset[0])>();
        return memory_size;
    }

    template <bool Q = UseShareMemory>
    typename std::enable_if<Q>::type
    Read(const boost::filesystem::path &path, void *begin, const void *end) const
    {
        const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
        storage::io::FileReader reader{path, fingerprint};

        begin = reader.DeserializeVector<typename decltype(weights)::value_type>(begin, end);
        begin =
            reader.DeserializeVector<typename decltype(source_boundary)::value_type>(begin, end);
        begin = reader.DeserializeVector<typename decltype(destination_boundary)::value_type>(begin,
                                                                                              end);
        begin = reader.DeserializeVector<typename decltype(cells)::value_type>(begin, end);
        begin = reader.DeserializeVector<typename decltype(level_to_cell_offset)::value_type>(begin,
                                                                                              end);
    }

    template <bool Q = UseShareMemory>
    typename std::enable_if<Q>::type InitializePointers(char *begin, const char *end)
    {
        auto weights_size = *reinterpret_cast<std::uint64_t *>(begin);
        begin += sizeof(weights_size);
        weights.reset(reinterpret_cast<EdgeWeight *>(begin), weights_size);
        begin += sizeof(decltype(weights[0])) * weights_size;

        auto source_boundary_size = *reinterpret_cast<std::uint64_t *>(begin);
        begin += sizeof(source_boundary_size);
        source_boundary.reset(reinterpret_cast<NodeID *>(begin), source_boundary_size);
        begin += sizeof(decltype(source_boundary[0])) * source_boundary_size;

        auto destination_boundary_size = *reinterpret_cast<std::uint64_t *>(begin);
        begin += sizeof(destination_boundary_size);
        destination_boundary.reset(reinterpret_cast<NodeID *>(begin), destination_boundary_size);
        begin += sizeof(decltype(destination_boundary[0])) * destination_boundary_size;

        auto cells_size = *reinterpret_cast<std::uint64_t *>(begin);
        begin += sizeof(cells_size);
        cells.reset(reinterpret_cast<CellData *>(begin), cells_size);
        begin += sizeof(decltype(cells[0])) * cells_size;

        auto level_to_cell_offset_size = *reinterpret_cast<std::uint64_t *>(begin);
        begin += sizeof(level_to_cell_offset_size);
        level_to_cell_offset.reset(reinterpret_cast<std::size_t *>(begin),
                                   level_to_cell_offset_size);
        begin += sizeof(decltype(level_to_cell_offset[0])) * level_to_cell_offset_size;

        BOOST_ASSERT(begin <= end);
    }

    template <bool Q = UseShareMemory>
    typename std::enable_if<!Q>::type Read(const boost::filesystem::path &path)
    {
        const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
        storage::io::FileReader reader{path, fingerprint};

        reader.DeserializeVector(weights);
        reader.DeserializeVector(source_boundary);
        reader.DeserializeVector(destination_boundary);
        reader.DeserializeVector(cells);
        reader.DeserializeVector(level_to_cell_offset);
    }

    void Write(const boost::filesystem::path &path) const
    {
        const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
        storage::io::FileWriter writer{path, fingerprint};

        writer.SerializeVector(weights);
        writer.SerializeVector(source_boundary);
        writer.SerializeVector(destination_boundary);
        writer.SerializeVector(cells);
        writer.SerializeVector(level_to_cell_offset);
    }

  private:
    typename util::ShM<EdgeWeight, UseShareMemory>::vector weights;
    typename util::ShM<NodeID, UseShareMemory>::vector source_boundary;
    typename util::ShM<NodeID, UseShareMemory>::vector destination_boundary;
    typename util::ShM<CellData, UseShareMemory>::vector cells;
    typename util::ShM<std::size_t, UseShareMemory>::vector level_to_cell_offset;
};
}
}

#endif
