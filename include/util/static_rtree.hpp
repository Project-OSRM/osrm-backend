#ifndef STATIC_RTREE_HPP
#define STATIC_RTREE_HPP

#include "storage/io.hpp"
#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/deallocating_vector.hpp"
#include "util/exception.hpp"
#include "util/integer_range.hpp"
#include "util/rectangle.hpp"
#include "util/typedefs.hpp"
#include "util/vector_view.hpp"
#include "util/web_mercator.hpp"

#include "osrm/coordinate.hpp"

#include "storage/shared_memory_ownership.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include <tbb/parallel_do.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <queue>
#include <string>
#include <vector>

// An extended alignment is implementation-defined, so use compiler attributes
// until alignas(LEAF_PAGE_SIZE) is compiler-independent.
#if defined(_MSC_VER)
#define ALIGNED(x) __declspec(align(x))
#elif defined(__GNUC__)
#define ALIGNED(x) __attribute__((aligned(x)))
#else
#define ALIGNED(x)
#endif

namespace osrm
{
namespace util
{

// Static RTree for serving nearest neighbour queries
// All coordinates are pojected first to Web Mercator before the bounding boxes
// are computed, this means the internal distance metric doesn not represent meters!
template <class EdgeDataT,
          storage::Ownership Ownership = storage::Ownership::Container,
          std::uint32_t BRANCHING_FACTOR = 64>
class StaticRTree
{
    template <typename T> using Vector = ViewOrVector<T, Ownership>;

  public:
    using Rectangle = RectangleInt2D;
    using EdgeData = EdgeDataT;
    using CoordinateList = Vector<util::Coordinate>;

    struct CandidateSegment
    {
        Coordinate fixed_projected_coordinate;
        EdgeDataT data;
    };

    struct TreeIndex
    {
        TreeIndex() : index(0), is_leaf(false) {}
        TreeIndex(std::size_t index, bool is_leaf) : index(index), is_leaf(is_leaf) {}
        std::uint32_t index : 31;
        std::uint32_t is_leaf : 1;
    };

    /**
     * TODO: Because the OMT tree packing is balanced and full, we
     *       can probably calculate the child count, leaf status
     *       and child index offset at query time, saving 8 bytes
     *       here.  I'm leaving this note here as a reminder for
     *       future work.
     */
    struct TreeNode
    {
        TreeNode() : child_count(0), is_leaf(false) {}
        std::uint32_t child_count : 31; // How many children this node has
        bool is_leaf : 1; // false means children are TreeNodes, true indicates children are edges
        std::uint32_t first_child_index; // offset of either the TreeNode in m_search_tree or the
                                         // edge data in m_edges, depending on is_leaf
        Rectangle minimum_bounding_rectangle; // the bounding box of this node
    };

  private:
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
              segment_index(segment_index), fixed_projected_coordinate(coordinate)
        {
        }

        inline bool is_segment() const
        {
            return segment_index != std::numeric_limits<std::uint32_t>::max();
        }

        inline bool operator<(const QueryCandidate &other) const
        {
            // Attn: this is reversed order. std::pq is a max pq!
            return other.squared_min_dist < squared_min_dist;
        }

        std::uint64_t squared_min_dist;
        TreeIndex tree_index;
        std::uint32_t segment_index;
        Coordinate fixed_projected_coordinate;
    };

    Vector<TreeNode> m_search_tree;
    const Vector<Coordinate> &m_coordinate_list;

    boost::iostreams::mapped_file_source m_edges_region;
    // read-only view of leaves
    util::vector_view<const EdgeDataT> m_edges;

  public:
    StaticRTree(const StaticRTree &) = delete;
    StaticRTree &operator=(const StaticRTree &) = delete;

    /**
     * Constructs an R-Tree using a modified OMT (Lee-Lee) approach.  Based on the rbush
     * library by Vladimir Agafonkin (https://github.com/mourner/rbush).
     *
     * The main characteristic of OMT packing is:
     *   The elements (EdgeDataT) are broken up into N groups, such that those
     *   groups can be evenly divided into a tree with degree BRANCHING_FACTOR.
     * The resulting tree has N branches from the root, then all subtrees use
     * BRANCHING_FACTOR branches.
     * This gives us even distribution of the data, and equal tree depth at all
     * leaf nodes.  It can be thought of as breaking up the data into "tiles"
     *
     * During construction, nodes are recursively sorted into first vertical columns,
     * then horizontal groups.  This produces efficient packing with minimal overlap.
     *
     * TODO: the packing is based on the centroid of each EdgeDataT - we could probably
     * do a little bit better if we considered the bounding box of the EdgeDataT instead.
     *
     * Tree structure
     * level
     *   0                   A
     *               ---------------------
     *             /       |       |       \   <- First level has N children
     *   1         B       C       D       E      (here N=4)
     *           /   \   /   \   /   \   /   \
     *   2       F   G   H   I   J   K   L   M  <- other levels have M chidren
     *          / \ / \ / \ / \ / \ / \ / \ / \    (here, M=2)
     *   3      N O P Q R S T U V W X Y Z 1 2 3
     *
     * TreeNodes get packed into a vector in BFS order like this:
     *  ABCDEFGHIJKLMNOPQRSTUVWXYZ123
     *
     * EdgeDataT is similarly packed - the children of the bottom nodes (N to 3)
     * are packed in order.
     *
     * @param edges the vector of EdgeDataT objects you want to insert into the tree.
     * @param tree_node_filename the file to write the TreeNode data to (the .ramIndex file).
     *                           This data will be later loaded into RAM.
     * @param leaf_node_filename the file to write the sorted EdgeDataT data to
     *                           (the .fileIndex file).  This data remains on disk and is read
     *                           on-demand at query time (via mmap()).
     * @param coordinate_list the actual coordinate data (EdgeDataT's u and v properties are
                              expected to be indexes into this vector)
     */
    explicit StaticRTree(std::vector<EdgeDataT> &&edges,
                         const std::string &tree_node_filename,
                         const std::string &leaf_node_filename,
                         const Vector<Coordinate> &coordinate_list)
        : m_coordinate_list(coordinate_list)
    {
        /**
         * This structure describes the tile of EdgeDataT that we're working on for
         * the current loop.
         */
        struct Tile
        {
            Tile(std::size_t parent_, std::size_t left_, std::size_t right_, std::size_t height_)
                : parent_treenode_index{parent_}, left_edge_index{left_}, right_edge_index{right_},
                  current_tree_height{height_}
            {
            }
            std::size_t parent_treenode_index;
            std::size_t left_edge_index;
            std::size_t right_edge_index;
            std::size_t current_tree_height;
        };

        // We create this tree using a queue, adding nodes to create to the end.
        // This means we end up with BFS sorted TreeNode and EdgeDataT lists.
        // See TODO notes on TreeNode for a future possible optimization where
        // we calculate list offsets dynamically, rather than by storing indexes
        // in TreeNodes.
        std::queue<Tile> queue;
        queue.emplace(0, 0, edges.size() - 1, 0);

        // TODO: we do a lot of sorting - it would make sense to only calculate centroids once
        //       or consider sorting by the east-most longitude
        auto longitude_compare = [this](const EdgeDataT &a, const EdgeDataT &b) {
            auto a_centroid =
                coordinate_calculation::centroid(m_coordinate_list[a.u], m_coordinate_list[a.v]);
            auto b_centroid =
                coordinate_calculation::centroid(m_coordinate_list[b.u], m_coordinate_list[b.v]);
            return a_centroid.lon < b_centroid.lon;
        };

        // TODO: same as other sort - we're probably doing this a lot more than necessary
        auto latitude_compare = [this](const EdgeDataT &a, const EdgeDataT &b) {
            auto a_centroid =
                coordinate_calculation::centroid(m_coordinate_list[a.u], m_coordinate_list[a.v]);
            auto b_centroid =
                coordinate_calculation::centroid(m_coordinate_list[b.u], m_coordinate_list[b.v]);
            return a_centroid.lat < b_centroid.lat;
        };

        {
            // Prepare the leaf_node_file for writing
            storage::io::FileWriter leaf_node_file(leaf_node_filename,
                                                   storage::io::FileWriter::HasNoFingerprint);

            // position of the last leaf node written to diskcountindex
            std::size_t saved_edges_count = 0;

            // Create the tree in breadth-first order
            while (!queue.empty())
            {
                auto current_tile = queue.front();
                queue.pop();

                // This is the number of EdgeDataT elements below the current tree node
                const auto number_of_edges =
                    current_tile.right_edge_index - current_tile.left_edge_index + 1;

                // The default number of children (not const because it's different for the root
                // node and leaf nodes)
                auto number_of_tiles = BRANCHING_FACTOR;

                // We've hit the bottom of the tree and we're building a leaf node
                if (number_of_edges <= number_of_tiles)
                {
                    TreeNode leaf_node;
                    leaf_node.is_leaf = true;
                    leaf_node.child_count = number_of_edges;

                    // Calculate the bounding-box for this leaf node based on the
                    // endpoints of the EdgeDataT values contained within
                    std::for_each(edges.begin() + current_tile.left_edge_index,
                                  edges.begin() + current_tile.left_edge_index + number_of_edges,
                                  [this, &leaf_node](const EdgeDataT &edge) {
                                      Coordinate projected_u{web_mercator::fromWGS84(
                                          Coordinate{m_coordinate_list[edge.u]})};
                                      Coordinate projected_v{web_mercator::fromWGS84(
                                          Coordinate{m_coordinate_list[edge.v]})};
                                      leaf_node.minimum_bounding_rectangle.Extend(projected_u.lon,
                                                                                  projected_u.lat);
                                      leaf_node.minimum_bounding_rectangle.Extend(projected_v.lon,
                                                                                  projected_v.lat);
                                  });

                    // We're appending to the edges data, so the current size is where our new data
                    // will be inserted.
                    leaf_node.first_child_index = saved_edges_count;
                    m_search_tree.push_back(leaf_node);

                    // If this node isn't also the root node, we need to update the parent child
                    // count and bounding box
                    if (current_tile.current_tree_height != 0)
                    {
                        // If this is the first child of the parent, we need to save the pointer
                        // to the current position in the m_search_tree array, as this is where
                        // the children for this parent begin
                        if (m_search_tree[current_tile.parent_treenode_index].child_count == 0)
                        {
                            m_search_tree[current_tile.parent_treenode_index].first_child_index =
                                m_search_tree.size() - 1;
                        }
                        ++m_search_tree[current_tile.parent_treenode_index].child_count;
                    }
                    // The root node can have > BRANCHING_FACTOR entries, but everone else
                    // should have max BRANCHING_FACTOR.
                    BOOST_ASSERT(current_tile.parent_treenode_index == 0 ||
                                 m_search_tree[current_tile.parent_treenode_index].child_count <=
                                     BRANCHING_FACTOR);

                    saved_edges_count += number_of_edges;
                    leaf_node_file.WriteFrom(edges.data() + current_tile.left_edge_index,
                                             number_of_edges);
                    continue;
                }

                // Add a new empty node to the back.  Note that the attributes on this newly
                // added node will be updated when we're processing it's children, we only insert
                // it as a placeholder at this time.
                m_search_tree.push_back(TreeNode());

                // Special case for the root node (height = 0),
                if (current_tile.current_tree_height == 0)
                {
                    // Calculate the final tree height
                    current_tile.current_tree_height =
                        std::ceil(std::log(number_of_edges) / std::log(number_of_tiles));
                    // Figure out the number of children needed at the root node
                    number_of_tiles =
                        std::ceil(number_of_edges /
                                  std::pow(number_of_tiles, current_tile.current_tree_height - 1));
                }
                else
                {
                    // This is a regular tree node
                    BOOST_ASSERT(m_search_tree[current_tile.parent_treenode_index].child_count <
                                 BRANCHING_FACTOR);
                    // Check to see if the parent node knows about the newly inserted item yet,
                    // if not, save the index of the current node as the first child offset
                    if (m_search_tree[current_tile.parent_treenode_index].child_count == 0)
                    {
                        m_search_tree[current_tile.parent_treenode_index].first_child_index =
                            m_search_tree.size() - 1;
                    }
                    // Increment the parent node's child count to include the node we're
                    // working on
                    ++m_search_tree[current_tile.parent_treenode_index].child_count;
                }

                // Calculate the number of entries in each tile for this iteration
                const std::size_t edges_per_tile =
                    std::ceil(static_cast<double>(number_of_edges) / number_of_tiles);
                std::size_t edges_per_column =
                    edges_per_tile * std::ceil(std::sqrt(number_of_tiles));

                // We need to save the indexes of each tile calculate so that we can
                // sort the items within
                typedef std::size_t EdgeIndex;
                std::vector<std::pair<EdgeIndex, EdgeIndex>> tiles;

                // Break the current tile up into columns
                for (auto column_start = current_tile.left_edge_index;
                     column_start <= current_tile.right_edge_index;
                     column_start += edges_per_column)
                {
                    auto column_end = std::min(column_start + edges_per_column - 1,
                                               current_tile.right_edge_index);
                    // Store the column start/end for sorting below
                    tiles.emplace_back(column_start, column_end);

                    // Now, break this column up into rows (edges_per_tile entries)
                    for (auto row_start = column_start; row_start <= column_end;
                         row_start += edges_per_tile)
                    {
                        auto row_end = std::min(row_start + edges_per_tile - 1, column_end);
                        // Queue up this range of edges for processing in a later loop
                        queue.emplace(m_search_tree.size() - 1,
                                      row_start,
                                      row_end,
                                      current_tile.current_tree_height - 1);
                    }
                }

                // Sort the current containing tile by longitude
                tbb::parallel_sort(edges.begin() + current_tile.left_edge_index,
                                   edges.begin() + current_tile.right_edge_index,
                                   longitude_compare);

                // Now, for each column we generated, sort those by latitude, so the data
                // is ready for the subsequent tile-by-tile processing in the next loop.
                tbb::parallel_do(
                    tiles.begin(), tiles.end(), [&](const std::pair<EdgeIndex, EdgeIndex> tile) {
                        tbb::parallel_sort(edges.begin() + tile.first,
                                           edges.begin() + tile.second,
                                           latitude_compare);
                    });
            }

            // Because we used a queue above, the m_search_tree vector is sorted in
            // the same order as a breadth-first-search of the tree.  We can iterate
            // over this in reverse and propogate node recangle sizes up the tree
            // The leaf nodes already have their bounding box set, so we just need to
            // propogate those up the tree
            std::for_each(m_search_tree.rbegin(), m_search_tree.rend(), [this](TreeNode &node) {
                if (node.child_count == 0 || node.is_leaf)
                {
                    return;
                }
                std::for_each(m_search_tree.begin() + node.first_child_index,
                              m_search_tree.begin() + node.first_child_index + node.child_count,
                              [&node](const TreeNode &child) {
                                  node.minimum_bounding_rectangle.MergeBoundingBoxes(
                                      child.minimum_bounding_rectangle);
                              });
            });

            // Note: leaf_node_file auto-closes at the end of the block
        }

        // Now, write out the tree nodes
        {
            storage::io::FileWriter tree_node_file(tree_node_filename,
                                                   storage::io::FileWriter::GenerateFingerprint);

            std::uint64_t size_of_tree = m_search_tree.size();
            BOOST_ASSERT_MSG(0 < size_of_tree, "tree empty");

            tree_node_file.WriteOne(size_of_tree);
            tree_node_file.WriteFrom(m_search_tree.data(), size_of_tree);
        }

        MapLeafNodesFile(leaf_node_filename);
    }

    explicit StaticRTree(const boost::filesystem::path &node_file,
                         const boost::filesystem::path &leaf_file,
                         const Vector<Coordinate> &coordinate_list)
        : m_coordinate_list(coordinate_list)
    {
        storage::io::FileReader tree_node_file(node_file,
                                               storage::io::FileReader::VerifyFingerprint);

        const auto tree_size = tree_node_file.ReadElementCount64();

        m_search_tree.resize(tree_size);
        tree_node_file.ReadInto(&m_search_tree[0], tree_size);

        MapLeafNodesFile(leaf_file);
    }

    explicit StaticRTree(TreeNode *tree_node_ptr,
                         const uint64_t number_of_nodes,
                         const boost::filesystem::path &leaf_file,
                         const Vector<Coordinate> &coordinate_list)
        : m_search_tree(tree_node_ptr, number_of_nodes), m_coordinate_list(coordinate_list)
    {
        MapLeafNodesFile(leaf_file);
    }

    void MapLeafNodesFile(const boost::filesystem::path &leaf_file)
    {
        // open leaf node file and return a pointer to the mapped leaves data
        try
        {
            m_edges_region.open(leaf_file);
            std::size_t num_edges = m_edges_region.size() / sizeof(EdgeDataT);
            auto data_ptr = m_edges_region.data();
            BOOST_ASSERT(reinterpret_cast<uintptr_t>(data_ptr) % alignof(EdgeDataT) == 0);
            m_edges.reset(reinterpret_cast<const EdgeDataT *>(data_ptr), num_edges);
        }
        catch (const std::exception &exc)
        {
            throw exception(boost::str(boost::format("Leaf file %1% mapping failed: %2%") %
                                       leaf_file % exc.what()) +
                            SOURCE_REF);
        }
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

        std::queue<std::uint32_t> traversal_queue;
        traversal_queue.push(0);

        while (!traversal_queue.empty())
        {
            auto const current_tree_index = traversal_queue.front();
            traversal_queue.pop();
            const TreeNode &current_tree_node = m_search_tree[current_tree_index];

            if (current_tree_node.is_leaf)
            {

                // TODO: we could potentially speed this up by having the current edge boxes
                // available in the leafnode directly
                std::for_each(m_edges.begin() + current_tree_node.first_child_index,
                              m_edges.begin() + current_tree_node.first_child_index +
                                  current_tree_node.child_count,
                              [&](const EdgeDataT &current_edge) {

                                  // we don't need to project the coordinates here,
                                  // because we use the unprojected rectangle to test
                                  // against
                                  const Rectangle bbox{
                                      std::min(m_coordinate_list[current_edge.u].lon,
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
                              });
            }
            else
            {
                std::accumulate(
                    m_search_tree.begin() + current_tree_node.first_child_index,
                    m_search_tree.begin() + current_tree_node.first_child_index +
                        current_tree_node.child_count,
                    current_tree_node.first_child_index,
                    [&](const std::uint32_t child_index, const TreeNode &child_node) {
                        if (child_node.minimum_bounding_rectangle.Intersects(projected_rectangle))
                        {
                            traversal_queue.push(child_index);
                        }
                        return child_index + 1;
                    });
            }
        }
        return results;
    }

    // Override filter and terminator for the desired behaviour.
    std::vector<EdgeDataT> Nearest(const Coordinate input_coordinate,
                                   const std::size_t max_results) const
    {
        return Nearest(input_coordinate,
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
        traversal_queue.push(QueryCandidate{0, TreeIndex{0, m_search_tree[0].is_leaf}});

        while (!traversal_queue.empty())
        {
            QueryCandidate current_query_node = traversal_queue.top();
            traversal_queue.pop();

            const TreeIndex &current_tree_index = current_query_node.tree_index;
            if (!current_query_node.is_segment())
            { // current object is a tree node
                if (current_tree_index.is_leaf)
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
                auto edge_data = m_edges[current_query_node.segment_index];
                const auto &current_candidate =
                    CandidateSegment{current_query_node.fixed_projected_coordinate, edge_data};

                // to allow returns of no-results if too restrictive filtering, this needs to be
                // done here even though performance would indicate that we want to stop after
                // adding the first candidate
                if (terminate(results.size(), current_candidate))
                {
                    break;
                }

                // Check if either directino of the segment passes our filter.  If not
                // continue to the next candidate
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
    template <typename QueueT>
    void ExploreLeafNode(const TreeIndex &leaf_id,
                         const Coordinate &projected_input_coordinate_fixed,
                         const FloatCoordinate &projected_input_coordinate,
                         QueueT &traversal_queue) const
    {
        const auto &current_leaf_node = m_search_tree[leaf_id.index];
        BOOST_ASSERT(current_leaf_node.is_leaf);

        // Using ::accumulate here instead of ::for_each so that we can get access to the
        // current
        // index being processed (current_edge_index).  This is basically the same
        std::accumulate(
            m_edges.begin() + current_leaf_node.first_child_index,
            m_edges.begin() + current_leaf_node.first_child_index + current_leaf_node.child_count,
            current_leaf_node.first_child_index,
            [&](std::uint32_t current_edge_index, const EdgeDataT &current_edge) {
                const auto projected_u = web_mercator::fromWGS84(m_coordinate_list[current_edge.u]);
                const auto projected_v = web_mercator::fromWGS84(m_coordinate_list[current_edge.v]);
                FloatCoordinate projected_nearest;
                std::tie(std::ignore, projected_nearest) =
                    coordinate_calculation::projectPointOnSegment(
                        projected_u, projected_v, projected_input_coordinate);

                const auto squared_distance = coordinate_calculation::squaredEuclideanDistance(
                    projected_input_coordinate_fixed, projected_nearest);
                BOOST_ASSERT(0. <= squared_distance);
                traversal_queue.push(
                    QueryCandidate{squared_distance,
                                   leaf_id,            // Index to the leaf node
                                   current_edge_index, // position of the edge in the big data dump
                                   Coordinate{projected_nearest}});

                return current_edge_index + 1;

            });
    }

    template <class QueueT>
    void ExploreTreeNode(const TreeIndex &tree_node_index,
                         const Coordinate &fixed_projected_input_coordinate,
                         QueueT &traversal_queue) const
    {
        const auto &tree_node = m_search_tree[tree_node_index.index];
        BOOST_ASSERT(tree_node.is_leaf == tree_node_index.is_leaf);
        BOOST_ASSERT(!tree_node.is_leaf);

        std::accumulate(m_search_tree.begin() + tree_node.first_child_index,
                        m_search_tree.begin() + tree_node.first_child_index + tree_node.child_count,
                        tree_node.first_child_index,
                        [&](const std::uint32_t child_index, const TreeNode &child) {

                            const auto squared_lower_bound_to_element =
                                child.minimum_bounding_rectangle.GetMinSquaredDist(
                                    fixed_projected_input_coordinate);

                            traversal_queue.push(
                                QueryCandidate{squared_lower_bound_to_element,
                                               TreeIndex{child_index, child.is_leaf}});

                            return child_index + 1;
                        });
    }
};

//[1] "On Packing R-Trees"; I. Kamel, C. Faloutsos; 1993; DOI: 10.1145/170088.170403
//[2] "Nearest Neighbor Queries", N. Roussopulos et al; 1995; DOI: 10.1145/223784.223794
//[3] "Distance Browsing in Spatial Databases"; G. Hjaltason, H. Samet; 1999; ACM Trans. DB
// Sys
// Vol.24 No.2, pp.265-318
}
}

#endif // STATIC_RTREE_HPP
