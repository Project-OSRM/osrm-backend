/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef STATICRTREE_H
#define STATICRTREE_H

#include "DeallocatingVector.h"
#include "HilbertValue.h"
#include "PhantomNodes.h"
#include "QueryNode.h"
#include "SharedMemoryFactory.h"
#include "SharedMemoryVectorWrapper.h"

#include "../Util/MercatorUtil.h"
#include "../Util/OSRMException.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/thread.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <memory>
#include <queue>
#include <string>
#include <vector>

// tuning parameters
const static uint32_t RTREE_BRANCHING_FACTOR = 64;
const static uint32_t RTREE_LEAF_NODE_SIZE = 1024;

static boost::thread_specific_ptr<boost::filesystem::ifstream> thread_local_rtree_stream;

// Implements a static, i.e. packed, R-tree
template <class DataT,
          class CoordinateListT = std::vector<FixedPointCoordinate>,
          bool UseSharedMemory = false>
class StaticRTree
{
  public:
    struct RectangleInt2D
    {
        RectangleInt2D() : min_lon(INT_MAX), max_lon(INT_MIN), min_lat(INT_MAX), max_lat(INT_MIN) {}

        int32_t min_lon, max_lon;
        int32_t min_lat, max_lat;

        inline void InitializeMBRectangle(const std::array<DataT, RTREE_LEAF_NODE_SIZE> &objects,
                                          const uint32_t element_count,
                                          const std::vector<NodeInfo> &coordinate_list)
        {
            for (uint32_t i = 0; i < element_count; ++i)
            {
                min_lon = std::min(min_lon,
                                   std::min(coordinate_list.at(objects[i].u).lon,
                                            coordinate_list.at(objects[i].v).lon));
                max_lon = std::max(max_lon,
                                   std::max(coordinate_list.at(objects[i].u).lon,
                                            coordinate_list.at(objects[i].v).lon));

                min_lat = std::min(min_lat,
                                   std::min(coordinate_list.at(objects[i].u).lat,
                                            coordinate_list.at(objects[i].v).lat));
                max_lat = std::max(max_lat,
                                   std::max(coordinate_list.at(objects[i].u).lat,
                                            coordinate_list.at(objects[i].v).lat));
            }
        }

        inline void AugmentMBRectangle(const RectangleInt2D &other)
        {
            min_lon = std::min(min_lon, other.min_lon);
            max_lon = std::max(max_lon, other.max_lon);
            min_lat = std::min(min_lat, other.min_lat);
            max_lat = std::max(max_lat, other.max_lat);
        }

        inline FixedPointCoordinate Centroid() const
        {
            FixedPointCoordinate centroid;
            // The coordinates of the midpoints are given by:
            // x = (x1 + x2) /2 and y = (y1 + y2) /2.
            centroid.lon = (min_lon + max_lon) / 2;
            centroid.lat = (min_lat + max_lat) / 2;
            return centroid;
        }

        inline bool Intersects(const RectangleInt2D &other) const
        {
            FixedPointCoordinate upper_left(other.max_lat, other.min_lon);
            FixedPointCoordinate upper_right(other.max_lat, other.max_lon);
            FixedPointCoordinate lower_right(other.min_lat, other.max_lon);
            FixedPointCoordinate lower_left(other.min_lat, other.min_lon);

            return (Contains(upper_left) || Contains(upper_right) || Contains(lower_right) ||
                    Contains(lower_left));
        }

        inline double GetMinDist(const FixedPointCoordinate &location) const
        {
            bool is_contained = Contains(location);
            if (is_contained)
            {
                return 0.;
            }

            double min_dist = std::numeric_limits<double>::max();
            min_dist = std::min(min_dist,
                                FixedPointCoordinate::ApproximateEuclideanDistance(
                                    location.lat, location.lon, max_lat, min_lon));
            min_dist = std::min(min_dist,
                                FixedPointCoordinate::ApproximateEuclideanDistance(
                                    location.lat, location.lon, max_lat, max_lon));
            min_dist = std::min(min_dist,
                                FixedPointCoordinate::ApproximateEuclideanDistance(
                                    location.lat, location.lon, min_lat, max_lon));
            min_dist = std::min(min_dist,
                                FixedPointCoordinate::ApproximateEuclideanDistance(
                                    location.lat, location.lon, min_lat, min_lon));
            return min_dist;
        }

        inline double GetMinMaxDist(const FixedPointCoordinate &location) const
        {
            double min_max_dist = std::numeric_limits<double>::max();
            // Get minmax distance to each of the four sides
            FixedPointCoordinate upper_left(max_lat, min_lon);
            FixedPointCoordinate upper_right(max_lat, max_lon);
            FixedPointCoordinate lower_right(min_lat, max_lon);
            FixedPointCoordinate lower_left(min_lat, min_lon);

            min_max_dist = std::min(
                min_max_dist,
                std::max(
                    FixedPointCoordinate::ApproximateEuclideanDistance(location, upper_left),
                    FixedPointCoordinate::ApproximateEuclideanDistance(location, upper_right)));

            min_max_dist = std::min(
                min_max_dist,
                std::max(
                    FixedPointCoordinate::ApproximateEuclideanDistance(location, upper_right),
                    FixedPointCoordinate::ApproximateEuclideanDistance(location, lower_right)));

            min_max_dist = std::min(
                min_max_dist,
                std::max(FixedPointCoordinate::ApproximateEuclideanDistance(location, lower_right),
                         FixedPointCoordinate::ApproximateEuclideanDistance(location, lower_left)));

            min_max_dist = std::min(
                min_max_dist,
                std::max(FixedPointCoordinate::ApproximateEuclideanDistance(location, lower_left),
                         FixedPointCoordinate::ApproximateEuclideanDistance(location, upper_left)));
            return min_max_dist;
        }

        inline bool Contains(const FixedPointCoordinate &location) const
        {
            const bool lats_contained = (location.lat > min_lat) && (location.lat < max_lat);
            const bool lons_contained = (location.lon > min_lon) && (location.lon < max_lon);
            return lats_contained && lons_contained;
        }

        inline friend std::ostream &operator<<(std::ostream &out, const RectangleInt2D &rect)
        {
            out << rect.min_lat / COORDINATE_PRECISION << "," << rect.min_lon / COORDINATE_PRECISION
                << " " << rect.max_lat / COORDINATE_PRECISION << ","
                << rect.max_lon / COORDINATE_PRECISION;
            return out;
        }
    };

    typedef RectangleInt2D RectangleT;

    struct TreeNode
    {
        TreeNode() : child_count(0), child_is_on_disk(false) {}
        RectangleT minimum_bounding_rectangle;
        uint32_t child_count : 31;
        bool child_is_on_disk : 1;
        uint32_t children[RTREE_BRANCHING_FACTOR];
    };

  private:
    struct WrappedInputElement
    {
        explicit WrappedInputElement(const uint32_t _array_index, const uint64_t _hilbert_value)
            : m_array_index(_array_index), m_hilbert_value(_hilbert_value)
        {
        }

        WrappedInputElement() : m_array_index(UINT_MAX), m_hilbert_value(0) {}

        uint32_t m_array_index;
        uint64_t m_hilbert_value;

        inline bool operator<(const WrappedInputElement &other) const
        {
            return m_hilbert_value < other.m_hilbert_value;
        }
    };

    struct LeafNode
    {
        LeafNode() : object_count(0) {}
        uint32_t object_count;
        std::array<DataT, RTREE_LEAF_NODE_SIZE> objects;
    };

    struct QueryCandidate
    {
        explicit QueryCandidate(const uint32_t n_id, const double dist)
            : node_id(n_id), min_dist(dist)
        {
        }
        QueryCandidate() : node_id(UINT_MAX), min_dist(std::numeric_limits<double>::max()) {}
        uint32_t node_id;
        double min_dist;
        inline bool operator<(const QueryCandidate &other) const
        {
            return min_dist < other.min_dist;
        }
    };

    typename ShM<TreeNode, UseSharedMemory>::vector m_search_tree;
    uint64_t m_element_count;
    const std::string m_leaf_node_filename;
    std::shared_ptr<CoordinateListT> m_coordinate_list;

  public:
    StaticRTree() = delete;
    StaticRTree(const StaticRTree &) = delete;

    // Construct a packed Hilbert-R-Tree with Kamel-Faloutsos algorithm [1]
    explicit StaticRTree(std::vector<DataT> &input_data_vector,
                         const std::string tree_node_filename,
                         const std::string leaf_node_filename,
                         const std::vector<NodeInfo> &coordinate_list)
        : m_element_count(input_data_vector.size()), m_leaf_node_filename(leaf_node_filename)
    {
        SimpleLogger().Write() << "constructing r-tree of " << m_element_count
                               << " edge elements build on-top of " << coordinate_list.size()
                               << " coordinates";

        std::chrono::time_point<std::chrono::steady_clock> time0 = std::chrono::steady_clock::now();
        std::vector<WrappedInputElement> input_wrapper_vector(m_element_count);

        HilbertCode get_hilbert_number;

// generate auxiliary vector of hilbert-values
#pragma omp parallel for schedule(guided)
        for (uint64_t element_counter = 0; element_counter < m_element_count; ++element_counter)
        {
            input_wrapper_vector[element_counter].m_array_index = element_counter;
            // Get Hilbert-Value for centroid in mercartor projection
            DataT const &current_element = input_data_vector[element_counter];
            FixedPointCoordinate current_centroid =
                DataT::Centroid(FixedPointCoordinate(coordinate_list.at(current_element.u).lat,
                                                     coordinate_list.at(current_element.u).lon),
                                FixedPointCoordinate(coordinate_list.at(current_element.v).lat,
                                                     coordinate_list.at(current_element.v).lon));
            current_centroid.lat =
                COORDINATE_PRECISION * lat2y(current_centroid.lat / COORDINATE_PRECISION);

            uint64_t current_hilbert_value = get_hilbert_number(current_centroid);
            input_wrapper_vector[element_counter].m_hilbert_value = current_hilbert_value;
        }

        // open leaf file
        boost::filesystem::ofstream leaf_node_file(leaf_node_filename, std::ios::binary);
        leaf_node_file.write((char *)&m_element_count, sizeof(uint64_t));

        // sort the hilbert-value representatives
        std::sort(input_wrapper_vector.begin(), input_wrapper_vector.end());
        std::vector<TreeNode> tree_nodes_in_level;

        // pack M elements into leaf node and write to leaf file
        uint64_t processed_objects_count = 0;
        while (processed_objects_count < m_element_count)
        {

            LeafNode current_leaf;
            TreeNode current_node;
            // SimpleLogger().Write() << "reading " << tree_size << " tree nodes in " <<
            // (sizeof(TreeNode)*tree_size) << " bytes";
            for (uint32_t current_element_index = 0; RTREE_LEAF_NODE_SIZE > current_element_index;
                 ++current_element_index)
            {
                if (m_element_count > (processed_objects_count + current_element_index))
                {
                    uint32_t index_of_next_object =
                        input_wrapper_vector[processed_objects_count + current_element_index]
                            .m_array_index;
                    current_leaf.objects[current_element_index] =
                        input_data_vector[index_of_next_object];
                    ++current_leaf.object_count;
                }
            }

            // generate tree node that resemble the objects in leaf and store it for next level
            current_node.minimum_bounding_rectangle.InitializeMBRectangle(
                current_leaf.objects, current_leaf.object_count, coordinate_list);
            current_node.child_is_on_disk = true;
            current_node.children[0] = tree_nodes_in_level.size();
            tree_nodes_in_level.emplace_back(current_node);

            // write leaf_node to leaf node file
            leaf_node_file.write((char *)&current_leaf, sizeof(current_leaf));
            processed_objects_count += current_leaf.object_count;
        }

        // close leaf file
        leaf_node_file.close();

        uint32_t processing_level = 0;
        while (1 < tree_nodes_in_level.size())
        {
            std::vector<TreeNode> tree_nodes_in_next_level;
            uint32_t processed_tree_nodes_in_level = 0;
            while (processed_tree_nodes_in_level < tree_nodes_in_level.size())
            {
                TreeNode parent_node;
                // pack RTREE_BRANCHING_FACTOR elements into tree_nodes each
                for (uint32_t current_child_node_index = 0;
                     RTREE_BRANCHING_FACTOR > current_child_node_index;
                     ++current_child_node_index)
                {
                    if (processed_tree_nodes_in_level < tree_nodes_in_level.size())
                    {
                        TreeNode &current_child_node =
                            tree_nodes_in_level[processed_tree_nodes_in_level];
                        // add tree node to parent entry
                        parent_node.children[current_child_node_index] = m_search_tree.size();
                        m_search_tree.emplace_back(current_child_node);
                        // augment MBR of parent
                        parent_node.minimum_bounding_rectangle.AugmentMBRectangle(
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

#pragma omp parallel for schedule(guided)
        for (uint32_t i = 0; i < m_search_tree.size(); ++i)
        {
            TreeNode &current_tree_node = m_search_tree[i];
            for (uint32_t j = 0; j < current_tree_node.child_count; ++j)
            {
                const uint32_t old_id = current_tree_node.children[j];
                const uint32_t new_id = m_search_tree.size() - old_id - 1;
                current_tree_node.children[j] = new_id;
            }
        }

        // open tree file
        boost::filesystem::ofstream tree_node_file(tree_node_filename, std::ios::binary);

        uint32_t size_of_tree = m_search_tree.size();
        BOOST_ASSERT_MSG(0 < size_of_tree, "tree empty");
        tree_node_file.write((char *)&size_of_tree, sizeof(uint32_t));
        tree_node_file.write((char *)&m_search_tree[0], sizeof(TreeNode) * size_of_tree);
        // close tree node file.
        tree_node_file.close();
        std::chrono::time_point<std::chrono::steady_clock> time1 = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = time1 - time0;
        SimpleLogger().Write() << "finished r-tree construction in " << (elapsed_seconds.count())
                               << " seconds";
    }

    // Read-only operation for queries
    explicit StaticRTree(const boost::filesystem::path &node_file,
                         const boost::filesystem::path &leaf_file,
                         const std::shared_ptr<CoordinateListT> coordinate_list)
        : m_leaf_node_filename(leaf_file.string())
    {
        // open tree node file and load into RAM.
        m_coordinate_list = coordinate_list;

        if (!boost::filesystem::exists(node_file))
        {
            throw OSRMException("ram index file does not exist");
        }
        if (0 == boost::filesystem::file_size(node_file))
        {
            throw OSRMException("ram index file is empty");
        }
        boost::filesystem::ifstream tree_node_file(node_file, std::ios::binary);

        uint32_t tree_size = 0;
        tree_node_file.read((char *)&tree_size, sizeof(uint32_t));

        m_search_tree.resize(tree_size);
        tree_node_file.read((char *)&m_search_tree[0], sizeof(TreeNode) * tree_size);
        tree_node_file.close();
        // open leaf node file and store thread specific pointer
        if (!boost::filesystem::exists(leaf_file))
        {
            throw OSRMException("mem index file does not exist");
        }
        if (0 == boost::filesystem::file_size(leaf_file))
        {
            throw OSRMException("mem index file is empty");
        }

        boost::filesystem::ifstream leaf_node_file(leaf_file, std::ios::binary);
        leaf_node_file.read((char *)&m_element_count, sizeof(uint64_t));
        leaf_node_file.close();

        // SimpleLogger().Write() << tree_size << " nodes in search tree";
        // SimpleLogger().Write() << m_element_count << " elements in leafs";
    }

    explicit StaticRTree(TreeNode *tree_node_ptr,
                         const uint32_t number_of_nodes,
                         const boost::filesystem::path &leaf_file,
                         std::shared_ptr<CoordinateListT> coordinate_list)
        : m_search_tree(tree_node_ptr, number_of_nodes), m_leaf_node_filename(leaf_file.string()),
          m_coordinate_list(coordinate_list)
    {
        // open leaf node file and store thread specific pointer
        if (!boost::filesystem::exists(leaf_file))
        {
            throw OSRMException("mem index file does not exist");
        }
        if (0 == boost::filesystem::file_size(leaf_file))
        {
            throw OSRMException("mem index file is empty");
        }

        boost::filesystem::ifstream leaf_node_file(leaf_file, std::ios::binary);
        leaf_node_file.read((char *)&m_element_count, sizeof(uint64_t));
        leaf_node_file.close();

        if (thread_local_rtree_stream.get())
        {
            thread_local_rtree_stream->close();
        }

        // SimpleLogger().Write() << tree_size << " nodes in search tree";
        // SimpleLogger().Write() << m_element_count << " elements in leafs";
    }
    // Read-only operation for queries

    bool LocateClosestEndPointForCoordinate(const FixedPointCoordinate &input_coordinate,
                                            FixedPointCoordinate &result_coordinate,
                                            const unsigned zoom_level)
    {
        bool ignore_tiny_components = (zoom_level <= 14);
        DataT nearest_edge;
        double min_dist = std::numeric_limits<double>::max();
        double min_max_dist = std::numeric_limits<double>::max();
        bool found_a_nearest_edge = false;

        // initialize queue with root element
        std::priority_queue<QueryCandidate> traversal_queue;
        double current_min_dist =
            m_search_tree[0].minimum_bounding_rectangle.GetMinDist(input_coordinate);
        traversal_queue.emplace(0, current_min_dist);

        while (!traversal_queue.empty())
        {
            const QueryCandidate current_query_node = traversal_queue.top();
            traversal_queue.pop();

            const bool prune_downward = (current_query_node.min_dist >= min_max_dist);
            const bool prune_upward = (current_query_node.min_dist >= min_dist);
            if (!prune_downward && !prune_upward)
            { // downward pruning
                TreeNode &current_tree_node = m_search_tree[current_query_node.node_id];
                if (current_tree_node.child_is_on_disk)
                {
                    LeafNode current_leaf_node;
                    LoadLeafFromDisk(current_tree_node.children[0], current_leaf_node);
                    for (uint32_t i = 0; i < current_leaf_node.object_count; ++i)
                    {
                        DataT const &current_edge = current_leaf_node.objects[i];
                        if (ignore_tiny_components && current_edge.is_in_tiny_cc)
                        {
                            continue;
                        }

                        double current_minimum_distance =
                            FixedPointCoordinate::ApproximateEuclideanDistance(
                                input_coordinate.lat,
                                input_coordinate.lon,
                                m_coordinate_list->at(current_edge.u).lat,
                                m_coordinate_list->at(current_edge.u).lon);
                        if (current_minimum_distance < min_dist)
                        {
                            // found a new minimum
                            min_dist = current_minimum_distance;
                            result_coordinate.lat = m_coordinate_list->at(current_edge.u).lat;
                            result_coordinate.lon = m_coordinate_list->at(current_edge.u).lon;
                            found_a_nearest_edge = true;
                        }

                        current_minimum_distance =
                            FixedPointCoordinate::ApproximateEuclideanDistance(
                                input_coordinate.lat,
                                input_coordinate.lon,
                                m_coordinate_list->at(current_edge.v).lat,
                                m_coordinate_list->at(current_edge.v).lon);

                        if (current_minimum_distance < min_dist)
                        {
                            // found a new minimum
                            min_dist = current_minimum_distance;
                            result_coordinate.lat = m_coordinate_list->at(current_edge.v).lat;
                            result_coordinate.lon = m_coordinate_list->at(current_edge.v).lon;
                            found_a_nearest_edge = true;
                        }
                    }
                }
                else
                {
                    // traverse children, prune if global mindist is smaller than local one
                    for (uint32_t i = 0; i < current_tree_node.child_count; ++i)
                    {
                        const int32_t child_id = current_tree_node.children[i];
                        const TreeNode &child_tree_node = m_search_tree[child_id];
                        const RectangleT &child_rectangle =
                            child_tree_node.minimum_bounding_rectangle;
                        const double current_min_dist =
                            child_rectangle.GetMinDist(input_coordinate);
                        const double current_min_max_dist =
                            child_rectangle.GetMinMaxDist(input_coordinate);
                        if (current_min_max_dist < min_max_dist)
                        {
                            min_max_dist = current_min_max_dist;
                        }
                        if (current_min_dist > min_max_dist)
                        {
                            continue;
                        }
                        if (current_min_dist > min_dist)
                        { // upward pruning
                            continue;
                        }
                        traversal_queue.emplace(child_id, current_min_dist);
                    }
                }
            }
        }

        return found_a_nearest_edge;
    }

    bool FindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                      PhantomNode &result_phantom_node,
                                      const unsigned zoom_level)
    {
        // SimpleLogger().Write() << "searching for coordinate " << input_coordinate;

        const bool ignore_tiny_components = (zoom_level <= 14);
        DataT nearest_edge;

        double min_dist = std::numeric_limits<double>::max();
        double min_max_dist = std::numeric_limits<double>::max();
        bool found_a_nearest_edge = false;

        FixedPointCoordinate nearest, current_start_coordinate, current_end_coordinate;

        // initialize queue with root element
        std::priority_queue<QueryCandidate> traversal_queue;
        double current_min_dist =
            m_search_tree[0].minimum_bounding_rectangle.GetMinDist(input_coordinate);
        traversal_queue.emplace(0, current_min_dist);

        BOOST_ASSERT_MSG(std::numeric_limits<double>::epsilon() >
                             (0. - traversal_queue.top().min_dist),
                         "Root element in NN Search has min dist != 0.");

        LeafNode current_leaf_node;
        while (!traversal_queue.empty())
        {
            const QueryCandidate current_query_node = traversal_queue.top();
            traversal_queue.pop();

            const bool prune_downward = (current_query_node.min_dist >= min_max_dist);
            const bool prune_upward = (current_query_node.min_dist >= min_dist);
            if (!prune_downward && !prune_upward)
            { // downward pruning
                const TreeNode &current_tree_node = m_search_tree[current_query_node.node_id];
                if (current_tree_node.child_is_on_disk)
                {
                    LoadLeafFromDisk(current_tree_node.children[0], current_leaf_node);
                    for (uint32_t i = 0; i < current_leaf_node.object_count; ++i)
                    {
                        DataT &current_edge = current_leaf_node.objects[i];
                        if (ignore_tiny_components && current_edge.is_in_tiny_cc)
                        {
                            continue;
                        }

                        double current_ratio = 0.;
                        const double current_perpendicular_distance =
                            FixedPointCoordinate::ComputePerpendicularDistance(
                                m_coordinate_list->at(current_edge.u),
                                m_coordinate_list->at(current_edge.v),
                                input_coordinate,
                                nearest,
                                current_ratio);

                        BOOST_ASSERT(0. <= current_perpendicular_distance);

                        if ((current_perpendicular_distance < min_dist) &&
                            !DoubleEpsilonCompare(current_perpendicular_distance, min_dist))
                        { // found a new minimum
                            min_dist = current_perpendicular_distance;
                            // TODO: use assignment c'tor in PhantomNode
                            result_phantom_node.forward_node_id =
                                current_edge.forward_edge_based_node_id;
                            result_phantom_node.reverse_node_id =
                                current_edge.reverse_edge_based_node_id;
                            result_phantom_node.name_id = current_edge.name_id;
                            result_phantom_node.forward_weight = current_edge.forward_weight;
                            result_phantom_node.reverse_weight = current_edge.reverse_weight;
                            result_phantom_node.forward_offset = current_edge.forward_offset;
                            result_phantom_node.reverse_offset = current_edge.reverse_offset;
                            result_phantom_node.packed_geometry_id =
                                current_edge.packed_geometry_id;
                            result_phantom_node.fwd_segment_position =
                                current_edge.fwd_segment_position;

                            result_phantom_node.location = nearest;
                            current_start_coordinate.lat =
                                m_coordinate_list->at(current_edge.u).lat;
                            current_start_coordinate.lon =
                                m_coordinate_list->at(current_edge.u).lon;
                            current_end_coordinate.lat = m_coordinate_list->at(current_edge.v).lat;
                            current_end_coordinate.lon = m_coordinate_list->at(current_edge.v).lon;
                            nearest_edge = current_edge;
                            found_a_nearest_edge = true;
                        }
                    }
                }
                else
                {
                    // traverse children, prune if global mindist is smaller than local one
                    for (uint32_t i = 0; i < current_tree_node.child_count; ++i)
                    {
                        const int32_t child_id = current_tree_node.children[i];
                        TreeNode &child_tree_node = m_search_tree[child_id];
                        RectangleT &child_rectangle = child_tree_node.minimum_bounding_rectangle;
                        const double current_min_dist =
                            child_rectangle.GetMinDist(input_coordinate);
                        const double current_min_max_dist =
                            child_rectangle.GetMinMaxDist(input_coordinate);
                        if (current_min_max_dist < min_max_dist)
                        {
                            min_max_dist = current_min_max_dist;
                        }
                        if (current_min_dist > min_max_dist)
                        {
                            continue;
                        }
                        if (current_min_dist > min_dist)
                        { // upward pruning
                            continue;
                        }
                        traversal_queue.emplace(child_id, current_min_dist);
                    }
                }
            }
        }

        // Hack to fix rounding errors and wandering via nodes.
        if (1 == std::abs(input_coordinate.lon - result_phantom_node.location.lon))
        {
            result_phantom_node.location.lon = input_coordinate.lon;
        }
        if (1 == std::abs(input_coordinate.lat - result_phantom_node.location.lat))
        {
            result_phantom_node.location.lat = input_coordinate.lat;
        }

        double ratio = 0.;

        if (found_a_nearest_edge)
        {
            const double distance_1 = FixedPointCoordinate::ApproximateEuclideanDistance(
                current_start_coordinate, result_phantom_node.location);

            const double distance_2 = FixedPointCoordinate::ApproximateEuclideanDistance(
                current_start_coordinate, current_end_coordinate);

            ratio = distance_1 / distance_2;
            ratio = std::min(1., ratio);

            if (SPECIAL_NODEID != result_phantom_node.forward_node_id)
            {
                result_phantom_node.forward_weight *= ratio;
            }
            if (SPECIAL_NODEID != result_phantom_node.reverse_node_id)
            {
                result_phantom_node.reverse_weight *= (1. - ratio);
            }
        }
        return found_a_nearest_edge;
    }

  private:
    inline void LoadLeafFromDisk(const uint32_t leaf_id, LeafNode &result_node)
    {
        if (!thread_local_rtree_stream.get() || !thread_local_rtree_stream->is_open())
        {
            thread_local_rtree_stream.reset(new boost::filesystem::ifstream(
                m_leaf_node_filename, std::ios::in | std::ios::binary));
        }
        if (!thread_local_rtree_stream->good())
        {
            thread_local_rtree_stream->clear(std::ios::goodbit);
            SimpleLogger().Write(logDEBUG) << "Resetting stale filestream";
        }
        uint64_t seek_pos = sizeof(uint64_t) + leaf_id * sizeof(LeafNode);
        thread_local_rtree_stream->seekg(seek_pos);
        thread_local_rtree_stream->read((char *)&result_node, sizeof(LeafNode));
    }

    inline bool EdgesAreEquivalent(const FixedPointCoordinate &a,
                                   const FixedPointCoordinate &b,
                                   const FixedPointCoordinate &c,
                                   const FixedPointCoordinate &d) const
    {
        return (a == b && c == d) || (a == c && b == d) || (a == d && b == c);
    }

    inline bool DoubleEpsilonCompare(const double d1, const double d2) const
    {
        return (std::abs(d1 - d2) < std::numeric_limits<double>::epsilon());
    }
};

//[1] "On Packing R-Trees"; I. Kamel, C. Faloutsos; 1993; DOI: 10.1145/170088.170403
//[2] "Nearest Neighbor Queries", N. Roussopulos et al; 1995; DOI: 10.1145/223784.223794

#endif // STATICRTREE_H
