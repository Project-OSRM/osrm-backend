#ifndef STATIC_RTREE_HPP
#define STATIC_RTREE_HPP

#include "storage/tar_fwd.hpp"

#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/deallocating_vector.hpp"
#include "util/exception.hpp"
#include "util/hilbert_value.hpp"
#include "util/integer_range.hpp"
#include "util/mmap_file.hpp"
#include "util/rectangle.hpp"
#include "util/typedefs.hpp"
#include "util/vector_view.hpp"
#include "util/web_mercator.hpp"

#include "osrm/coordinate.hpp"

#include "storage/shared_memory_ownership.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace osrm
{
namespace util
{
template <class EdgeDataT,
          storage::Ownership Ownership = storage::Ownership::Container,
          std::uint32_t BRANCHING_FACTOR = 64,
          std::uint32_t LEAF_PAGE_SIZE = 4096>
class StaticRTree;

namespace serialization
{
template <class EdgeDataT,
          storage::Ownership Ownership,
          std::uint32_t BRANCHING_FACTOR,
          std::uint32_t LEAF_PAGE_SIZE>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 util::StaticRTree<EdgeDataT, Ownership, BRANCHING_FACTOR, LEAF_PAGE_SIZE> &rtree);

template <class EdgeDataT,
          storage::Ownership Ownership,
          std::uint32_t BRANCHING_FACTOR,
          std::uint32_t LEAF_PAGE_SIZE>
inline void
write(storage::tar::FileWriter &writer,
      const std::string &name,
      const util::StaticRTree<EdgeDataT, Ownership, BRANCHING_FACTOR, LEAF_PAGE_SIZE> &rtree);
} // namespace serialization

/***
 * Static RTree for serving nearest neighbour queries
 * // All coordinates are pojected first to Web Mercator before the bounding boxes
 * // are computed, this means the internal distance metric doesn not represent meters!
 */

template <class EdgeDataT,
          storage::Ownership Ownership,
          std::uint32_t BRANCHING_FACTOR,
          std::uint32_t LEAF_PAGE_SIZE>
class StaticRTree
{
    /**********************************************************
     * Example RTree construction:
     *
     * 30 elements (EdgeDataT objects)
     * LEAF_NODE_SIZE = 3
     * BRANCHING_FACTOR = 2
     *
     * 012 345 678 901 234 567 890 123 456 789  <- EdgeDataT objects in .fileIndex data, sorted by
     * \|/ \|/ \|/ \|/ \|/ \|/ \|/ \|/ \|/ \|/     Hilbert Code of the centroid coordinate
     * A   B   C   D   E   F   G   H   I   J   <- Everything from here down is a Rectangle in
     * \ /     \ /     \ /     \ /     \ /        .ramIndex
     *  K       L       M       N       O
     *   \     /         \     /       /
     *    \   /           \   /       /
     *     \ /             \ /       /
     *      P               Q       R
     *       \             /       /
     *        \           /       /
     *         \         /       /
     *          \       /       /
     *           \     /       /
     *            \   /       /
     *             \ /       /
     *              U       V
     *               \     /
     *                \   /
     *                 \ /
     *                  W
     *
     * Step 1 - objects 01234567... are sorted by Hilbert code (these are the line
     *          segments of the OSM roads)
     * Step 2 - we grab LEAF_NODE_SIZE of them at a time and create TreeNode A with a
     *          bounding-box that surrounds the first LEAF_NODE_SIZE objects
     * Step 2a- continue grabbing LEAF_NODE_SIZE objects, creating TreeNodes B,C,D,E...J
     *          until we run out of objects.  The last TreeNode J may not have
     *          LEAF_NODE_SIZE entries.  Our math later on caters for this.
     * Step 3 - Now start grabbing nodes from A..J in groups of BRANCHING_FACTOR,
     *          and create K..O with bounding boxes surrounding the groups of
     *          BRANCHING_FACTOR.  Again, O, the last entry, may have fewer than
     *          BRANCHING_FACTOR entries.
     * Step 3a- Repeat this process for each level, until you only create 1 TreeNode
     *          to contain its children (in this case, W).
     *
     * As we create TreeNodes, we append them to the m_search_tree vector.
     *
     * After this part of the building process, m_search_tree will contain TreeNode
     * objects in this order:
     *
     * ABCDEFGHIJ KLMNO PQR UV W
     * 10         5     3   2  1  <- number of nodes in the level
     *
     * In order to make our math easy later on, we reverse the whole array,
     * then reverse the nodes within each level:
     *
     *   Reversed:        W VU RQP ONMKL JIHGFEDCBA
     *   Levels reversed: W UV PQR KLMNO ABCDEFGHIJ
     *
     * We also now have the following information:
     *
     *   level sizes = {1,2,3,5,10}
     *
     * and we can calculate the array position the nodes for each level
     * start (based on the sum of the previous level sizes):
     *
     *   level starts = {0,1,3,6,11}
     *
     * Now, some basic math can be used to navigate around the tree.  See
     * the body of the `child_indexes` function for the details.
     *
     ***********************************************/
    template <typename T> using Vector = ViewOrVector<T, Ownership>;

  public:
    using Rectangle = RectangleInt2D;
    using EdgeData = EdgeDataT;
    using CoordinateList = Vector<util::Coordinate>;

    static_assert(LEAF_PAGE_SIZE >= sizeof(EdgeDataT), "page size is too small");
    static_assert(((LEAF_PAGE_SIZE - 1) & LEAF_PAGE_SIZE) == 0, "page size is not a power of 2");
    static constexpr std::uint32_t LEAF_NODE_SIZE = (LEAF_PAGE_SIZE / sizeof(EdgeDataT));

    struct CandidateSegment
    {
        Coordinate fixed_projected_coordinate;
        EdgeDataT data;
    };

    /**
     * Represents a node position somewhere in our tree.  This is purely a navigation
     * class used to find children of each node - the actual data for each node
     * is in the m_search_tree vector of TreeNode objects.
     */
    struct TreeIndex
    {
        TreeIndex() : level(0), offset(0) {}
        TreeIndex(std::uint32_t level_, std::uint32_t offset_) : level(level_), offset(offset_) {}
        std::uint32_t level;  // Which level of the tree is this node in
        std::uint32_t offset; // Which node on this level is this (0=leftmost)
    };

    /**
     * An actual node in the tree.  It's pretty minimal, we use the TreeIndex
     * classes to navigate around.  The TreeNode is packed into m_search_tree
     * in a specific order so we can calculate positions of children
     * (see the children_indexes function)
     */
    struct TreeNode
    {
        Rectangle minimum_bounding_rectangle;
    };

  private:
    /**
     * A lightweight wrapper for the Hilbert Code for each EdgeDataT object
     * A vector of these is used to sort the EdgeDataT input onto the
     * Hilbert Curve.
     * The sorting doesn't modify the original array, so this struct
     * maintains a pointer to the original index position (m_original_index)
     * so we can fetch the original data from the sorted position.
     */
    struct WrappedInputElement
    {
        explicit WrappedInputElement(const uint64_t _hilbert_value,
                                     const std::uint32_t _original_index)
            : m_hilbert_value(_hilbert_value), m_original_index(_original_index)
        {
        }

        WrappedInputElement() : m_hilbert_value(0), m_original_index(UINT_MAX) {}

        uint64_t m_hilbert_value;
        std::uint32_t m_original_index;

        inline bool operator<(const WrappedInputElement &other) const
        {
            return m_hilbert_value < other.m_hilbert_value;
        }
    };

    struct QueryCandidate
    {
        QueryCandidate(std::uint64_t squared_min_dist, TreeIndex tree_index)
            : squared_min_dist(squared_min_dist), tree_index(tree_index),
              segment_index(std::numeric_limits<std::uint32_t>::max())
        {
        }

        QueryCandidate(std::uint64_t squared_min_dist,
                       TreeIndex tree_index,
                       std::uint32_t segment_index,
                       const Coordinate &coordinate)
            : squared_min_dist(squared_min_dist), tree_index(tree_index),
              fixed_projected_coordinate(coordinate), segment_index(segment_index)
        {
        }

        inline bool is_segment() const
        {
            return segment_index != std::numeric_limits<std::uint32_t>::max();
        }

        inline bool operator<(const QueryCandidate &other) const
        {
            // Attn: this is reversed order. std::priority_queue is a
            // max pq (biggest item at the front)!
            return other.squared_min_dist < squared_min_dist;
        }

        std::uint64_t squared_min_dist;
        TreeIndex tree_index;
        Coordinate fixed_projected_coordinate;
        std::uint32_t segment_index;
    };

    // Representation of the in-memory search tree
    Vector<TreeNode> m_search_tree;
    // Reference to the actual lon/lat data we need for doing math
    util::vector_view<const Coordinate> m_coordinate_list;
    // Holds the start indexes of each level in m_search_tree
    Vector<std::uint64_t> m_tree_level_starts;
    // mmap'd .fileIndex file
    boost::iostreams::mapped_file_source m_objects_region;
    // This is a view of the EdgeDataT data mmap'd from the .fileIndex file
    util::vector_view<const EdgeDataT> m_objects;

  public:
    StaticRTree() = default;
    StaticRTree(const StaticRTree &) = delete;
    StaticRTree &operator=(const StaticRTree &) = delete;
    StaticRTree(StaticRTree &&) = default;
    StaticRTree &operator=(StaticRTree &&) = default;

    // Construct a packed Hilbert-R-Tree with Kamel-Faloutsos algorithm [1]
    explicit StaticRTree(const std::vector<EdgeDataT> &input_data_vector,
                         const Vector<Coordinate> &coordinate_list,
                         const boost::filesystem::path &on_disk_file_name)
        : m_coordinate_list(coordinate_list.data(), coordinate_list.size())
    {
        const auto element_count = input_data_vector.size();
        std::vector<WrappedInputElement> input_wrapper_vector(element_count);

        // Step 1 - create a vector of Hilbert Code/original position pairs
        tbb::parallel_for(
            tbb::blocked_range<uint64_t>(0, element_count),
            [&input_data_vector, &input_wrapper_vector, this](
                const tbb::blocked_range<uint64_t> &range) {
                for (uint64_t element_counter = range.begin(), end = range.end();
                     element_counter != end;
                     ++element_counter)
                {
                    WrappedInputElement &current_wrapper = input_wrapper_vector[element_counter];
                    current_wrapper.m_original_index = element_counter;

                    EdgeDataT const &current_element = input_data_vector[element_counter];

                    // Get Hilbert-Value for centroid in mercartor projection
                    BOOST_ASSERT(current_element.u < m_coordinate_list.size());
                    BOOST_ASSERT(current_element.v < m_coordinate_list.size());

                    Coordinate current_centroid = coordinate_calculation::centroid(
                        m_coordinate_list[current_element.u], m_coordinate_list[current_element.v]);
                    current_centroid.lat = FixedLatitude{static_cast<std::int32_t>(
                        COORDINATE_PRECISION *
                        web_mercator::latToY(toFloating(current_centroid.lat)))};

                    current_wrapper.m_hilbert_value = GetHilbertCode(current_centroid);
                }
            });

        // sort the hilbert-value representatives
        tbb::parallel_sort(input_wrapper_vector.begin(), input_wrapper_vector.end());
        {
            boost::iostreams::mapped_file out_objects_region;
            auto out_objects = mmapFile<EdgeDataT>(on_disk_file_name,
                                                   out_objects_region,
                                                   input_data_vector.size() * sizeof(EdgeDataT));

            // Note, we can't just write everything in one go, because the input_data_vector
            // is not sorted by hilbert code, only the input_wrapper_vector is in the correct
            // order.  Instead, we iterate over input_wrapper_vector, copy the hilbert-indexed
            // entries from input_data_vector into a temporary contiguous array, then write
            // that array to disk.

            // Create the first level of TreeNodes - each bounding LEAF_NODE_COUNT EdgeDataT
            // objects.
            std::size_t wrapped_element_index = 0;
            auto objects_iter = out_objects.begin();

            while (wrapped_element_index < element_count)
            {
                TreeNode current_node;

                // Loop over the next block of EdgeDataT, calculate the bounding box
                // for the block, and save the data to write to disk in the correct
                // order.
                for (std::uint32_t object_index = 0;
                     object_index < LEAF_NODE_SIZE && wrapped_element_index < element_count;
                     ++object_index, ++wrapped_element_index)
                {
                    const std::uint32_t input_object_index =
                        input_wrapper_vector[wrapped_element_index].m_original_index;
                    const EdgeDataT &object = input_data_vector[input_object_index];

                    *objects_iter++ = object;

                    Coordinate projected_u{
                        web_mercator::fromWGS84(Coordinate{m_coordinate_list[object.u]})};
                    Coordinate projected_v{
                        web_mercator::fromWGS84(Coordinate{m_coordinate_list[object.v]})};

                    BOOST_ASSERT(std::abs(toFloating(projected_u.lon).operator double()) <= 180.);
                    BOOST_ASSERT(std::abs(toFloating(projected_u.lat).operator double()) <= 180.);
                    BOOST_ASSERT(std::abs(toFloating(projected_v.lon).operator double()) <= 180.);
                    BOOST_ASSERT(std::abs(toFloating(projected_v.lat).operator double()) <= 180.);

                    Rectangle rectangle;
                    rectangle.min_lon =
                        std::min(rectangle.min_lon, std::min(projected_u.lon, projected_v.lon));
                    rectangle.max_lon =
                        std::max(rectangle.max_lon, std::max(projected_u.lon, projected_v.lon));

                    rectangle.min_lat =
                        std::min(rectangle.min_lat, std::min(projected_u.lat, projected_v.lat));
                    rectangle.max_lat =
                        std::max(rectangle.max_lat, std::max(projected_u.lat, projected_v.lat));

                    BOOST_ASSERT(rectangle.IsValid());
                    current_node.minimum_bounding_rectangle.MergeBoundingBoxes(rectangle);
                }

                m_search_tree.emplace_back(current_node);
            }
        }
        // mmap as read-only now
        m_objects = mmapFile<EdgeDataT>(on_disk_file_name, m_objects_region);

        // Should hold the number of nodes at the lowest level of the graph (closest
        // to the data)
        std::uint32_t nodes_in_previous_level = m_search_tree.size();

        // Holds the number of TreeNodes in each level.
        // We always start with the root node, so
        // m_tree_level_sizes[0] should always be 1
        std::vector<std::uint64_t> tree_level_sizes;
        tree_level_sizes.push_back(nodes_in_previous_level);

        // Now, repeatedly create levels of nodes that contain BRANCHING_FACTOR
        // nodes from the previous level.
        while (nodes_in_previous_level > 1)
        {
            auto previous_level_start_pos = m_search_tree.size() - nodes_in_previous_level;

            // We can calculate how many nodes will be in this level, we divide by
            // BRANCHING_FACTOR
            // and round up
            std::uint32_t nodes_in_current_level =
                std::ceil(static_cast<double>(nodes_in_previous_level) / BRANCHING_FACTOR);

            for (auto current_node_idx : irange<std::size_t>(0, nodes_in_current_level))
            {
                TreeNode parent_node;
                auto first_child_index =
                    current_node_idx * BRANCHING_FACTOR + previous_level_start_pos;
                auto last_child_index =
                    first_child_index +
                    std::min<std::size_t>(BRANCHING_FACTOR,
                                          nodes_in_previous_level -
                                              current_node_idx * BRANCHING_FACTOR);

                // Calculate the bounding box for BRANCHING_FACTOR nodes in the previous
                // level, then save that box as a new TreeNode in the new level.
                for (auto child_node_idx : irange<std::size_t>(first_child_index, last_child_index))
                {
                    parent_node.minimum_bounding_rectangle.MergeBoundingBoxes(
                        m_search_tree[child_node_idx].minimum_bounding_rectangle);
                }
                m_search_tree.emplace_back(parent_node);
            }
            nodes_in_previous_level = nodes_in_current_level;
            tree_level_sizes.push_back(nodes_in_previous_level);
        }
        // At this point, we've got our tree built, but the nodes are in a weird order.
        // Next thing we'll do is flip it around so that we don't end up with a lot of
        // `size - n` math later on.

        // Flip the tree so that the root node is at 0.
        // This just makes our math during search a bit more intuitive
        std::reverse(m_search_tree.begin(), m_search_tree.end());

        // Same for the level sizes - root node / base level is at 0
        std::reverse(tree_level_sizes.begin(), tree_level_sizes.end());

        // The first level starts at 0
        m_tree_level_starts = {0};
        // The remaining levels start at the partial sum of the preceeding level sizes
        std::partial_sum(tree_level_sizes.begin(),
                         tree_level_sizes.end(),
                         std::back_inserter(m_tree_level_starts));
        BOOST_ASSERT(m_tree_level_starts.size() >= 2);

        // Now we have to flip the coordinates within each level so that math is easier
        // later on.  The workflow here is:
        // The initial order of tree nodes in the m_search_tree array is roughly:
        // 6789 345 12 0   (each block here is a level of the tree)
        // Then we reverse it and get:
        // 0 21 543 9876
        // Now the loop below reverses each level to give us the final result
        // 0 12 345 6789
        // This ordering keeps the position math easy to understand during later
        // searches
        for (auto i : irange<std::size_t>(0, tree_level_sizes.size()))
        {
            std::reverse(m_search_tree.begin() + m_tree_level_starts[i],
                         m_search_tree.begin() + m_tree_level_starts[i] + tree_level_sizes[i]);
        }
    }

    /**
     * Constructs an empty RTree for de-serialization.
     */
    template <typename = std::enable_if<Ownership == storage::Ownership::Container>>
    explicit StaticRTree(const boost::filesystem::path &on_disk_file_name,
                         const Vector<Coordinate> &coordinate_list)
        : m_coordinate_list(coordinate_list.data(), coordinate_list.size())
    {
        m_objects = mmapFile<EdgeDataT>(on_disk_file_name, m_objects_region);
    }

    /**
     * Constructs an r-tree from blocks of memory loaded by someone else
     * (usually a shared memory block created by osrm-datastore)
     * These memory blocks basically just contain the files read into RAM,
     * excep the .fileIndex file always stays on disk, and we mmap() it as usual
     */
    explicit StaticRTree(Vector<TreeNode> search_tree_,
                         Vector<std::uint64_t> tree_level_starts,
                         const boost::filesystem::path &on_disk_file_name,
                         const Vector<Coordinate> &coordinate_list)
        : m_search_tree(std::move(search_tree_)),
          m_coordinate_list(coordinate_list.data(), coordinate_list.size()),
          m_tree_level_starts(std::move(tree_level_starts))
    {
        BOOST_ASSERT(m_tree_level_starts.size() >= 2);
        m_objects = mmapFile<EdgeDataT>(on_disk_file_name, m_objects_region);
    }

    /* Returns all features inside the bounding box.
       Rectangle needs to be projected!*/
    std::vector<EdgeDataT> SearchInBox(const Rectangle &search_rectangle) const
    {
        const Rectangle projected_rectangle{
            search_rectangle.min_lon,
            search_rectangle.max_lon,
            toFixed(FloatLatitude{
                web_mercator::latToY(toFloating(FixedLatitude(search_rectangle.min_lat)))}),
            toFixed(FloatLatitude{
                web_mercator::latToY(toFloating(FixedLatitude(search_rectangle.max_lat)))})};
        std::vector<EdgeDataT> results;

        std::queue<TreeIndex> traversal_queue;
        traversal_queue.push(TreeIndex{});

        while (!traversal_queue.empty())
        {
            auto const current_tree_index = traversal_queue.front();
            traversal_queue.pop();

            // If we're at the bottom of the tree, we need to explore the
            // element array
            if (is_leaf(current_tree_index))
            {

                // Note: irange is [start,finish), so we need to +1 to make sure we visit the
                // last
                for (const auto current_child_index : child_indexes(current_tree_index))
                {
                    const auto &current_edge = m_objects[current_child_index];

                    // we don't need to project the coordinates here,
                    // because we use the unprojected rectangle to test against
                    const Rectangle bbox{std::min(m_coordinate_list[current_edge.u].lon,
                                                  m_coordinate_list[current_edge.v].lon),
                                         std::max(m_coordinate_list[current_edge.u].lon,
                                                  m_coordinate_list[current_edge.v].lon),
                                         std::min(m_coordinate_list[current_edge.u].lat,
                                                  m_coordinate_list[current_edge.v].lat),
                                         std::max(m_coordinate_list[current_edge.u].lat,
                                                  m_coordinate_list[current_edge.v].lat)};

                    // use the _unprojected_ input rectangle here
                    if (bbox.Intersects(search_rectangle))
                    {
                        results.push_back(current_edge);
                    }
                }
            }
            else
            {
                BOOST_ASSERT(current_tree_index.level + 1 < m_tree_level_starts.size());

                for (const auto child_index : child_indexes(current_tree_index))
                {
                    const auto &child_rectangle =
                        m_search_tree[child_index].minimum_bounding_rectangle;

                    if (child_rectangle.Intersects(projected_rectangle))
                    {
                        traversal_queue.push(TreeIndex(
                            current_tree_index.level + 1,
                            child_index - m_tree_level_starts[current_tree_index.level + 1]));
                    }
                }
            }
        }
        return results;
    }

    // Override filter and terminator for the desired behaviour.
    std::vector<EdgeDataT> Nearest(const Coordinate input_coordinate,
                                   const std::size_t max_results) const
    {
        return Nearest(
            input_coordinate,
            [](const CandidateSegment &) { return std::make_pair(true, true); },
            [max_results](const std::size_t num_results, const CandidateSegment &) {
                return num_results >= max_results;
            });
    }

    // Override filter and terminator for the desired behaviour.
    template <typename FilterT, typename TerminationT>
    std::vector<EdgeDataT> Nearest(const Coordinate input_coordinate,
                                   const FilterT filter,
                                   const TerminationT terminate) const
    {
        std::vector<EdgeDataT> results;
        auto projected_coordinate = web_mercator::fromWGS84(input_coordinate);
        Coordinate fixed_projected_coordinate{projected_coordinate};
        // initialize queue with root element
        std::priority_queue<QueryCandidate> traversal_queue;
        traversal_queue.push(QueryCandidate{0, TreeIndex{}});

        while (!traversal_queue.empty())
        {
            QueryCandidate current_query_node = traversal_queue.top();
            traversal_queue.pop();

            const TreeIndex &current_tree_index = current_query_node.tree_index;
            if (!current_query_node.is_segment())
            { // current object is a tree node
                if (is_leaf(current_tree_index))
                {
                    ExploreLeafNode(current_tree_index,
                                    fixed_projected_coordinate,
                                    projected_coordinate,
                                    traversal_queue);
                }
                else
                {
                    ExploreTreeNode(
                        current_tree_index, fixed_projected_coordinate, traversal_queue);
                }
            }
            else
            { // current candidate is an actual road segment
                // We deliberatly make a copy here, we mutate the value below
                auto edge_data = m_objects[current_query_node.segment_index];
                const auto &current_candidate =
                    CandidateSegment{current_query_node.fixed_projected_coordinate, edge_data};

                // to allow returns of no-results if too restrictive filtering, this needs to be
                // done here even though performance would indicate that we want to stop after
                // adding the first candidate
                if (terminate(results.size(), current_candidate))
                {
                    break;
                }

                auto use_segment = filter(current_candidate);
                if (!use_segment.first && !use_segment.second)
                {
                    continue;
                }
                edge_data.forward_segment_id.enabled &= use_segment.first;
                edge_data.reverse_segment_id.enabled &= use_segment.second;

                // store phantom node in result vector
                results.push_back(std::move(edge_data));
            }
        }

        return results;
    }

  private:
    /**
     * Iterates over all the objects in a leaf node and inserts them into our
     * search priority queue.  The speed of this function is very much governed
     * by the value of LEAF_NODE_SIZE, as we'll calculate the euclidean distance
     * for every child of each leaf node visited.
     */
    template <typename QueueT>
    void ExploreLeafNode(const TreeIndex &leaf_id,
                         const Coordinate &projected_input_coordinate_fixed,
                         const FloatCoordinate &projected_input_coordinate,
                         QueueT &traversal_queue) const
    {
        // Check that we're actually looking at the bottom level of the tree
        BOOST_ASSERT(is_leaf(leaf_id));

        for (const auto i : child_indexes(leaf_id))
        {
            const auto &current_edge = m_objects[i];

            const auto projected_u = web_mercator::fromWGS84(m_coordinate_list[current_edge.u]);
            const auto projected_v = web_mercator::fromWGS84(m_coordinate_list[current_edge.v]);

            FloatCoordinate projected_nearest;
            std::tie(std::ignore, projected_nearest) =
                coordinate_calculation::projectPointOnSegment(
                    projected_u, projected_v, projected_input_coordinate);

            const auto squared_distance = coordinate_calculation::squaredEuclideanDistance(
                projected_input_coordinate_fixed, projected_nearest);
            // distance must be non-negative
            BOOST_ASSERT(0. <= squared_distance);
            BOOST_ASSERT(i < std::numeric_limits<std::uint32_t>::max());
            traversal_queue.push(QueryCandidate{squared_distance,
                                                leaf_id,
                                                static_cast<std::uint32_t>(i),
                                                Coordinate{projected_nearest}});
        }
    }

    /**
     * Iterates over all the children of a TreeNode and inserts them into the search
     * priority queue using their distance from the search coordinate as the
     * priority metric.
     * The closests distance to a box from our point is also the closest distance
     * to the closest line in that box (assuming the boxes hug their contents).
     */
    template <class QueueT>
    void ExploreTreeNode(const TreeIndex &parent,
                         const Coordinate &fixed_projected_input_coordinate,
                         QueueT &traversal_queue) const
    {
        // Figure out which_id level the parent is on, and it's offset
        // in that level.
        // Check that we're actually looking at the bottom level of the tree
        BOOST_ASSERT(!is_leaf(parent));

        for (const auto child_index : child_indexes(parent))
        {
            const auto &child = m_search_tree[child_index];

            const auto squared_lower_bound_to_element =
                child.minimum_bounding_rectangle.GetMinSquaredDist(
                    fixed_projected_input_coordinate);

            traversal_queue.push(QueryCandidate{
                squared_lower_bound_to_element,
                TreeIndex(parent.level + 1, child_index - m_tree_level_starts[parent.level + 1])});
        }
    }

    std::uint64_t GetLevelSize(const std::size_t level) const
    {
        BOOST_ASSERT(m_tree_level_starts.size() > level + 1);
        BOOST_ASSERT(m_tree_level_starts[level + 1] >= m_tree_level_starts[level]);
        return m_tree_level_starts[level + 1] - m_tree_level_starts[level];
    }

    /**
     * Calculates the absolute position of child data in our packed data
     * vectors.
     *
     * when given a TreeIndex that is a leaf node (i.e. at the bottom of the tree),
     * this function returns indexes valid for `m_objects`
     *
     * otherwise, the indexes are to be used with m_search_tree to iterate over
     * the children of `parent`
     *
     * This function assumes we pack nodes as described in the big comment
     * at the top of this class.  All nodes are fully filled except for the last
     * one in each level.
     */
    range<std::size_t> child_indexes(const TreeIndex &parent) const
    {
        // If we're looking at a leaf node, the index is from 0 to m_objects.size(),
        // there is only 1 level of object data in the m_objects array
        if (is_leaf(parent))
        {
            const std::uint64_t first_child_index = parent.offset * LEAF_NODE_SIZE;
            const std::uint64_t end_child_index = std::min(
                first_child_index + LEAF_NODE_SIZE, static_cast<std::uint64_t>(m_objects.size()));

            BOOST_ASSERT(first_child_index < std::numeric_limits<std::uint32_t>::max());
            BOOST_ASSERT(end_child_index < std::numeric_limits<std::uint32_t>::max());
            BOOST_ASSERT(end_child_index <= m_objects.size());

            return irange<std::size_t>(first_child_index, end_child_index);
        }
        else
        {
            const std::uint64_t first_child_index =
                m_tree_level_starts[parent.level + 1] + parent.offset * BRANCHING_FACTOR;

            const std::uint64_t end_child_index =
                std::min(first_child_index + BRANCHING_FACTOR,
                         m_tree_level_starts[parent.level + 1] + GetLevelSize(parent.level + 1));
            BOOST_ASSERT(first_child_index < std::numeric_limits<std::uint32_t>::max());
            BOOST_ASSERT(end_child_index < std::numeric_limits<std::uint32_t>::max());
            BOOST_ASSERT(end_child_index <= m_search_tree.size());
            BOOST_ASSERT(end_child_index <=
                         m_tree_level_starts[parent.level + 1] + GetLevelSize(parent.level + 1));
            return irange<std::size_t>(first_child_index, end_child_index);
        }
    }

    bool is_leaf(const TreeIndex &treeindex) const
    {
        BOOST_ASSERT(m_tree_level_starts.size() >= 2);
        return treeindex.level == m_tree_level_starts.size() - 2;
    }

    friend void serialization::read<EdgeDataT, Ownership, BRANCHING_FACTOR, LEAF_PAGE_SIZE>(
        storage::tar::FileReader &reader, const std::string &name, StaticRTree &rtree);

    friend void serialization::write<EdgeDataT, Ownership, BRANCHING_FACTOR, LEAF_PAGE_SIZE>(
        storage::tar::FileWriter &writer, const std::string &name, const StaticRTree &rtree);
};

//[1] "On Packing R-Trees"; I. Kamel, C. Faloutsos; 1993; DOI: 10.1145/170088.170403
//[2] "Nearest Neighbor Queries", N. Roussopulos et al; 1995; DOI: 10.1145/223784.223794
//[3] "Distance Browsing in Spatial Databases"; G. Hjaltason, H. Samet; 1999; ACM Trans. DB Sys
// Vol.24 No.2, pp.265-318
} // namespace util
} // namespace osrm

#endif // STATIC_RTREE_HPP
