#ifndef STATIC_RTREE_HPP
#define STATIC_RTREE_HPP

#include "util/deallocating_vector.hpp"
#include "util/hilbert_value.hpp"
#include "util/rectangle.hpp"
#include "util/shared_memory_vector_wrapper.hpp"
#include "util/bearing.hpp"
#include "util/exception.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/web_mercator.hpp"

#include "osrm/coordinate.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#include <variant/variant.hpp>

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

// Static RTree for serving nearest neighbour queries
// All coordinates are pojected first to Web Mercator before the bounding boxes
// are computed, this means the internal distance metric doesn not represent meters!
template <class EdgeDataT,
          class CoordinateListT = std::vector<Coordinate>,
          bool UseSharedMemory = false,
          std::uint32_t BRANCHING_FACTOR = 64,
          std::uint32_t LEAF_NODE_SIZE = 256>
class StaticRTree
{
  public:
    using Rectangle = RectangleInt2D;
    using EdgeData = EdgeDataT;
    using CoordinateList = CoordinateListT;

    struct CandidateSegment
    {
        Coordinate fixed_projected_coordinate;
        EdgeDataT data;
    };

    struct TreeNode
    {
        TreeNode() : child_count(0), child_is_on_disk(false) {}
        Rectangle minimum_bounding_rectangle;
        std::uint32_t child_count : 31;
        bool child_is_on_disk : 1;
        std::uint32_t children[BRANCHING_FACTOR];
    };

    struct LeafNode
    {
        LeafNode() : object_count(0), objects() {}
        uint32_t object_count;
        std::array<EdgeDataT, LEAF_NODE_SIZE> objects;
    };

  private:
    struct WrappedInputElement
    {
        explicit WrappedInputElement(const uint64_t _hilbert_value,
                                     const std::uint32_t _array_index)
            : m_hilbert_value(_hilbert_value), m_array_index(_array_index)
        {
        }

        WrappedInputElement() : m_hilbert_value(0), m_array_index(UINT_MAX) {}

        uint64_t m_hilbert_value;
        std::uint32_t m_array_index;

        inline bool operator<(const WrappedInputElement &other) const
        {
            return m_hilbert_value < other.m_hilbert_value;
        }
    };

    using QueryNodeType = mapbox::util::variant<std::uint32_t, CandidateSegment>;
    struct QueryCandidate
    {
        inline bool operator<(const QueryCandidate &other) const
        {
            // Attn: this is reversed order. std::pq is a max pq!
            return other.squared_min_dist < squared_min_dist;
        }

        std::uint64_t squared_min_dist;
        QueryNodeType node;
    };

    typename ShM<TreeNode, UseSharedMemory>::vector m_search_tree;
    uint64_t m_element_count;
    const std::string m_leaf_node_filename;
    std::shared_ptr<CoordinateListT> m_coordinate_list;
    boost::filesystem::ifstream leaves_stream;

  public:
    StaticRTree(const StaticRTree &) = delete;
    StaticRTree &operator=(const StaticRTree &) = delete;

    template <typename CoordinateT>
    // Construct a packed Hilbert-R-Tree with Kamel-Faloutsos algorithm [1]
    explicit StaticRTree(const std::vector<EdgeDataT> &input_data_vector,
                         const std::string &tree_node_filename,
                         const std::string &leaf_node_filename,
                         const std::vector<CoordinateT> &coordinate_list)
        : m_element_count(input_data_vector.size()), m_leaf_node_filename(leaf_node_filename)
    {
        std::vector<WrappedInputElement> input_wrapper_vector(m_element_count);

        // generate auxiliary vector of hilbert-values
        tbb::parallel_for(
            tbb::blocked_range<uint64_t>(0, m_element_count),
            [&input_data_vector, &input_wrapper_vector,
             &coordinate_list](const tbb::blocked_range<uint64_t> &range)
            {
                for (uint64_t element_counter = range.begin(), end = range.end();
                     element_counter != end; ++element_counter)
                {
                    WrappedInputElement &current_wrapper = input_wrapper_vector[element_counter];
                    current_wrapper.m_array_index = element_counter;

                    EdgeDataT const &current_element = input_data_vector[element_counter];

                    // Get Hilbert-Value for centroid in mercartor projection
                    BOOST_ASSERT(current_element.u < coordinate_list.size());
                    BOOST_ASSERT(current_element.v < coordinate_list.size());

                    Coordinate current_centroid = coordinate_calculation::centroid(
                        coordinate_list[current_element.u], coordinate_list[current_element.v]);
                    current_centroid.lat =
                        FixedLatitude(COORDINATE_PRECISION *
                                      web_mercator::latToY(toFloating(current_centroid.lat)));

                    current_wrapper.m_hilbert_value = hilbertCode(current_centroid);
                }
            });

        // open leaf file
        boost::filesystem::ofstream leaf_node_file(leaf_node_filename, std::ios::binary);
        leaf_node_file.write((char *)&m_element_count, sizeof(uint64_t));

        // sort the hilbert-value representatives
        tbb::parallel_sort(input_wrapper_vector.begin(), input_wrapper_vector.end());
        std::vector<TreeNode> tree_nodes_in_level;

        // pack M elements into leaf node and write to leaf file
        uint64_t processed_objects_count = 0;
        while (processed_objects_count < m_element_count)
        {

            LeafNode current_leaf;
            TreeNode current_node;
            for (std::uint32_t current_element_index = 0; LEAF_NODE_SIZE > current_element_index;
                 ++current_element_index)
            {
                if (m_element_count > (processed_objects_count + current_element_index))
                {
                    std::uint32_t index_of_next_object =
                        input_wrapper_vector[processed_objects_count + current_element_index]
                            .m_array_index;
                    current_leaf.objects[current_element_index] =
                        input_data_vector[index_of_next_object];
                    ++current_leaf.object_count;
                }
            }

            // generate tree node that resemble the objects in leaf and store it for next level
            InitializeMBRectangle(current_node.minimum_bounding_rectangle, current_leaf.objects,
                                  current_leaf.object_count, coordinate_list);
            current_node.child_is_on_disk = true;
            current_node.children[0] = tree_nodes_in_level.size();
            tree_nodes_in_level.emplace_back(current_node);

            // write leaf_node to leaf node file
            leaf_node_file.write((char *)&current_leaf, sizeof(current_leaf));
            processed_objects_count += current_leaf.object_count;
        }

        std::uint32_t processing_level = 0;
        while (1 < tree_nodes_in_level.size())
        {
            std::vector<TreeNode> tree_nodes_in_next_level;
            std::uint32_t processed_tree_nodes_in_level = 0;
            while (processed_tree_nodes_in_level < tree_nodes_in_level.size())
            {
                TreeNode parent_node;
                // pack BRANCHING_FACTOR elements into tree_nodes each
                for (std::uint32_t current_child_node_index = 0;
                     BRANCHING_FACTOR > current_child_node_index; ++current_child_node_index)
                {
                    if (processed_tree_nodes_in_level < tree_nodes_in_level.size())
                    {
                        TreeNode &current_child_node =
                            tree_nodes_in_level[processed_tree_nodes_in_level];
                        // add tree node to parent entry
                        parent_node.children[current_child_node_index] = m_search_tree.size();
                        m_search_tree.emplace_back(current_child_node);
                        // merge MBRs
                        parent_node.minimum_bounding_rectangle.MergeBoundingBoxes(
                            current_child_node.minimum_bounding_rectangle);
                        // increase counters
                        ++parent_node.child_count;
                        ++processed_tree_nodes_in_level;
                    }
                }
                tree_nodes_in_next_level.emplace_back(parent_node);
            }
            tree_nodes_in_level.swap(tree_nodes_in_next_level);
            ++processing_level;
        }
        BOOST_ASSERT_MSG(1 == tree_nodes_in_level.size(), "tree broken, more than one root node");
        // last remaining entry is the root node, store it
        m_search_tree.emplace_back(tree_nodes_in_level[0]);

        // reverse and renumber tree to have root at index 0
        std::reverse(m_search_tree.begin(), m_search_tree.end());

        std::uint32_t search_tree_size = m_search_tree.size();
        tbb::parallel_for(tbb::blocked_range<std::uint32_t>(0, search_tree_size),
                          [this, &search_tree_size](const tbb::blocked_range<std::uint32_t> &range)
                          {
                              for (std::uint32_t i = range.begin(), end = range.end(); i != end;
                                   ++i)
                              {
                                  TreeNode &current_tree_node = this->m_search_tree[i];
                                  for (std::uint32_t j = 0; j < current_tree_node.child_count; ++j)
                                  {
                                      const std::uint32_t old_id = current_tree_node.children[j];
                                      const std::uint32_t new_id = search_tree_size - old_id - 1;
                                      current_tree_node.children[j] = new_id;
                                  }
                              }
                          });

        // open tree file
        boost::filesystem::ofstream tree_node_file(tree_node_filename, std::ios::binary);

        std::uint32_t size_of_tree = m_search_tree.size();
        BOOST_ASSERT_MSG(0 < size_of_tree, "tree empty");
        tree_node_file.write((char *)&size_of_tree, sizeof(std::uint32_t));
        tree_node_file.write((char *)&m_search_tree[0], sizeof(TreeNode) * size_of_tree);
    }

    explicit StaticRTree(const boost::filesystem::path &node_file,
                         const boost::filesystem::path &leaf_file,
                         const std::shared_ptr<CoordinateListT> coordinate_list)
        : m_leaf_node_filename(leaf_file.string())
    {
        // open tree node file and load into RAM.
        m_coordinate_list = coordinate_list;

        if (!boost::filesystem::exists(node_file))
        {
            throw exception("ram index file does not exist");
        }
        if (0 == boost::filesystem::file_size(node_file))
        {
            throw exception("ram index file is empty");
        }
        boost::filesystem::ifstream tree_node_file(node_file, std::ios::binary);

        std::uint32_t tree_size = 0;
        tree_node_file.read((char *)&tree_size, sizeof(std::uint32_t));

        m_search_tree.resize(tree_size);
        if (tree_size > 0)
        {
            tree_node_file.read((char *)&m_search_tree[0], sizeof(TreeNode) * tree_size);
        }

        // open leaf node file and store thread specific pointer
        if (!boost::filesystem::exists(leaf_file))
        {
            throw exception("mem index file does not exist");
        }
        if (0 == boost::filesystem::file_size(leaf_file))
        {
            throw exception("mem index file is empty");
        }

        leaves_stream.open(leaf_file, std::ios::binary);
        leaves_stream.read((char *)&m_element_count, sizeof(uint64_t));
    }

    explicit StaticRTree(TreeNode *tree_node_ptr,
                         const uint64_t number_of_nodes,
                         const boost::filesystem::path &leaf_file,
                         std::shared_ptr<CoordinateListT> coordinate_list)
        : m_search_tree(tree_node_ptr, number_of_nodes), m_leaf_node_filename(leaf_file.string()),
          m_coordinate_list(std::move(coordinate_list))
    {
        // open leaf node file and store thread specific pointer
        if (!boost::filesystem::exists(leaf_file))
        {
            throw exception("mem index file does not exist");
        }
        if (0 == boost::filesystem::file_size(leaf_file))
        {
            throw exception("mem index file is empty");
        }

        leaves_stream.open(leaf_file, std::ios::binary);
        leaves_stream.read((char *)&m_element_count, sizeof(uint64_t));
    }

    /* Returns all features inside the bounding box.
       Rectangle needs to be projected!*/
    std::vector<EdgeDataT> SearchInBox(const Rectangle &search_rectangle)
    {
        const Rectangle projected_rectangle{
            search_rectangle.min_lon, search_rectangle.max_lon,
            toFixed(FloatLatitude{
                web_mercator::latToY(toFloating(FixedLatitude(search_rectangle.min_lat)))}),
            toFixed(FloatLatitude{
                web_mercator::latToY(toFloating(FixedLatitude(search_rectangle.max_lat)))})};
        std::vector<EdgeDataT> results;

        std::queue<TreeNode> traversal_queue;

        traversal_queue.push(m_search_tree[0]);

        while (!traversal_queue.empty())
        {
            auto const current_tree_node = traversal_queue.front();
            traversal_queue.pop();

            if (current_tree_node.child_is_on_disk)
            {
                LeafNode current_leaf_node;
                LoadLeafFromDisk(current_tree_node.children[0], current_leaf_node);

                for (const auto i : irange(0u, current_leaf_node.object_count))
                {
                    const auto &current_edge = current_leaf_node.objects[i];

                    // we don't need to project the coordinates here,
                    // because we use the unprojected rectangle to test against
                    const Rectangle bbox{std::min((*m_coordinate_list)[current_edge.u].lon,
                                                  (*m_coordinate_list)[current_edge.v].lon),
                                         std::max((*m_coordinate_list)[current_edge.u].lon,
                                                  (*m_coordinate_list)[current_edge.v].lon),
                                         std::min((*m_coordinate_list)[current_edge.u].lat,
                                                  (*m_coordinate_list)[current_edge.v].lat),
                                         std::max((*m_coordinate_list)[current_edge.u].lat,
                                                  (*m_coordinate_list)[current_edge.v].lat)};

                    // use the _unprojected_ input rectangle here
                    if (bbox.Intersects(search_rectangle))
                    {
                        results.push_back(current_edge);
                    }
                }
            }
            else
            {
                // If it's a tree node, look at all children and add them
                // to the search queue if their bounding boxes intersect
                for (std::uint32_t i = 0; i < current_tree_node.child_count; ++i)
                {
                    const std::int32_t child_id = current_tree_node.children[i];
                    const auto &child_tree_node = m_search_tree[child_id];
                    const auto &child_rectangle = child_tree_node.minimum_bounding_rectangle;

                    if (child_rectangle.Intersects(projected_rectangle))
                    {
                        traversal_queue.push(m_search_tree[child_id]);
                    }
                }
            }
        }
        return results;
    }

    // Override filter and terminator for the desired behaviour.
    std::vector<EdgeDataT> Nearest(const Coordinate input_coordinate, const std::size_t max_results)
    {
        return Nearest(input_coordinate,
                       [](const CandidateSegment &)
                       {
                           return std::make_pair(true, true);
                       },
                       [max_results](const std::size_t num_results, const CandidateSegment &)
                       {
                           return num_results >= max_results;
                       });
    }

    // Override filter and terminator for the desired behaviour.
    template <typename FilterT, typename TerminationT>
    std::vector<EdgeDataT>
    Nearest(const Coordinate input_coordinate, const FilterT filter, const TerminationT terminate)
    {
        std::vector<EdgeDataT> results;
        auto projected_coordinate = web_mercator::fromWGS84(input_coordinate);
        Coordinate fixed_projected_coordinate{projected_coordinate};

        // initialize queue with root element
        std::priority_queue<QueryCandidate> traversal_queue;
        traversal_queue.push(QueryCandidate{0, 0});

        while (!traversal_queue.empty())
        {
            QueryCandidate current_query_node = traversal_queue.top();
            traversal_queue.pop();

            if (current_query_node.node.template is<std::uint32_t>())
            { // current object is a tree node
                const TreeNode &current_tree_node =
                    m_search_tree[current_query_node.node.template get<std::uint32_t>()];
                if (current_tree_node.child_is_on_disk)
                {
                    ExploreLeafNode(current_tree_node.children[0], fixed_projected_coordinate,
                                    projected_coordinate, traversal_queue);
                }
                else
                {
                    ExploreTreeNode(current_tree_node, fixed_projected_coordinate, traversal_queue);
                }
            }
            else
            {
                // inspecting an actual road segment
                auto &current_candidate = current_query_node.node.template get<CandidateSegment>();

                // to allow returns of no-results if too restrictive filtering, this needs to be done here
                // even though performance would indicate that we want to stop after adding the first candidate
                if (terminate(results.size(), current_candidate))
                {
                    traversal_queue = std::priority_queue<QueryCandidate>{};
                    break;
                }

                auto use_segment = filter(current_candidate);
                if (!use_segment.first && !use_segment.second)
                {
                    continue;
                }
                current_candidate.data.forward_segment_id.enabled &= use_segment.first;
                current_candidate.data.reverse_segment_id.enabled &= use_segment.second;

                // store phantom node in result vector
                results.push_back(std::move(current_candidate.data));
            }
        }

        return results;
    }

  private:
    template <typename QueueT>
    void ExploreLeafNode(const std::uint32_t leaf_id,
                         const Coordinate projected_input_coordinate_fixed,
                         const FloatCoordinate &projected_input_coordinate,
                         QueueT &traversal_queue)
    {
        LeafNode current_leaf_node;
        LoadLeafFromDisk(leaf_id, current_leaf_node);

        // current object represents a block on disk
        for (const auto i : irange(0u, current_leaf_node.object_count))
        {
            const auto &current_edge = current_leaf_node.objects[i];
            const auto projected_u = web_mercator::fromWGS84((*m_coordinate_list)[current_edge.u]);
            const auto projected_v = web_mercator::fromWGS84((*m_coordinate_list)[current_edge.v]);

            FloatCoordinate projected_nearest;
            std::tie(std::ignore, projected_nearest) =
                coordinate_calculation::projectPointOnSegment(projected_u, projected_v,
                                                              projected_input_coordinate);

            const auto squared_distance = coordinate_calculation::squaredEuclideanDistance(
                projected_input_coordinate_fixed, projected_nearest);
            // distance must be non-negative
            BOOST_ASSERT(0. <= squared_distance);
            traversal_queue.push(
                QueryCandidate{squared_distance, CandidateSegment{Coordinate{projected_nearest},
                                                                  std::move(current_edge)}});
        }
    }

    template <class QueueT>
    void ExploreTreeNode(const TreeNode &parent,
                         const Coordinate fixed_projected_input_coordinate,
                         QueueT &traversal_queue)
    {
        for (std::uint32_t i = 0; i < parent.child_count; ++i)
        {
            const std::int32_t child_id = parent.children[i];
            const auto &child_tree_node = m_search_tree[child_id];
            const auto &child_rectangle = child_tree_node.minimum_bounding_rectangle;
            const auto squared_lower_bound_to_element =
                child_rectangle.GetMinSquaredDist(fixed_projected_input_coordinate);
            traversal_queue.push(
                QueryCandidate{squared_lower_bound_to_element, child_id});
        }
    }

    inline void LoadLeafFromDisk(const std::uint32_t leaf_id, LeafNode &result_node)
    {
        if (!leaves_stream.is_open())
        {
            leaves_stream.open(m_leaf_node_filename, std::ios::in | std::ios::binary);
        }
        if (!leaves_stream.good())
        {
            throw exception("Could not read from leaf file.");
        }
        const uint64_t seek_pos = sizeof(uint64_t) + leaf_id * sizeof(LeafNode);
        leaves_stream.seekg(seek_pos);
        BOOST_ASSERT_MSG(leaves_stream.good(), "Seeking to position in leaf file failed.");
        leaves_stream.read((char *)&result_node, sizeof(LeafNode));
        BOOST_ASSERT_MSG(leaves_stream.good(), "Reading from leaf file failed.");
    }

    template <typename CoordinateT>
    void InitializeMBRectangle(Rectangle &rectangle,
                               const std::array<EdgeDataT, LEAF_NODE_SIZE> &objects,
                               const std::uint32_t element_count,
                               const std::vector<CoordinateT> &coordinate_list)
    {
        for (std::uint32_t i = 0; i < element_count; ++i)
        {
            BOOST_ASSERT(objects[i].u < coordinate_list.size());
            BOOST_ASSERT(objects[i].v < coordinate_list.size());

            Coordinate projected_u{
                web_mercator::fromWGS84(Coordinate{coordinate_list[objects[i].u]})};
            Coordinate projected_v{
                web_mercator::fromWGS84(Coordinate{coordinate_list[objects[i].v]})};

            BOOST_ASSERT(toFloating(projected_u.lon) <= FloatLongitude(180.));
            BOOST_ASSERT(toFloating(projected_u.lon) >= FloatLongitude(-180.));
            BOOST_ASSERT(toFloating(projected_u.lat) <= FloatLatitude(180.));
            BOOST_ASSERT(toFloating(projected_u.lat) >= FloatLatitude(-180.));
            BOOST_ASSERT(toFloating(projected_v.lon) <= FloatLongitude(180.));
            BOOST_ASSERT(toFloating(projected_v.lon) >= FloatLongitude(-180.));
            BOOST_ASSERT(toFloating(projected_v.lat) <= FloatLatitude(180.));
            BOOST_ASSERT(toFloating(projected_v.lat) >= FloatLatitude(-180.));

            rectangle.min_lon =
                std::min(rectangle.min_lon, std::min(projected_u.lon, projected_v.lon));
            rectangle.max_lon =
                std::max(rectangle.max_lon, std::max(projected_u.lon, projected_v.lon));

            rectangle.min_lat =
                std::min(rectangle.min_lat, std::min(projected_u.lat, projected_v.lat));
            rectangle.max_lat =
                std::max(rectangle.max_lat, std::max(projected_u.lat, projected_v.lat));
        }
        BOOST_ASSERT(rectangle.min_lon != FixedLongitude(std::numeric_limits<int>::min()));
        BOOST_ASSERT(rectangle.min_lat != FixedLatitude(std::numeric_limits<int>::min()));
        BOOST_ASSERT(rectangle.max_lon != FixedLongitude(std::numeric_limits<int>::min()));
        BOOST_ASSERT(rectangle.max_lat != FixedLatitude(std::numeric_limits<int>::min()));
    }
};

//[1] "On Packing R-Trees"; I. Kamel, C. Faloutsos; 1993; DOI: 10.1145/170088.170403
//[2] "Nearest Neighbor Queries", N. Roussopulos et al; 1995; DOI: 10.1145/223784.223794
//[3] "Distance Browsing in Spatial Databases"; G. Hjaltason, H. Samet; 1999; ACM Trans. DB Sys
// Vol.24 No.2, pp.265-318
}
}

#endif // STATIC_RTREE_HPP
