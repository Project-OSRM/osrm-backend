/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef STATICRTREE_H_
#define STATICRTREE_H_

#include "MercatorUtil.h"
#include "Coordinate.h"
#include "PhantomNodes.h"
#include "DeallocatingVector.h"
#include "HilbertValue.h"
#include "../Util/OSRMException.h"
#include "../Util/SimpleLogger.h"
#include "../Util/TimingUtil.h"
#include "../typedefs.h"

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/minmax.hpp>
#include <boost/algorithm/minmax_element.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#include <cassert>
#include <cfloat>
#include <climits>

#include <algorithm>
#include <queue>
#include <string>
#include <vector>

//tuning parameters
const static uint32_t RTREE_BRANCHING_FACTOR = 50;
const static uint32_t RTREE_LEAF_NODE_SIZE = 1170;

// Implements a static, i.e. packed, R-tree

static boost::thread_specific_ptr<boost::filesystem::ifstream> thread_local_rtree_stream;

template<class DataT>
class StaticRTree : boost::noncopyable {
private:
    struct RectangleInt2D {
        RectangleInt2D() :
            min_lon(INT_MAX),
            max_lon(INT_MIN),
            min_lat(INT_MAX),
            max_lat(INT_MIN) {}

        int32_t min_lon, max_lon;
        int32_t min_lat, max_lat;

        inline void InitializeMBRectangle(
                const DataT * objects,
                const uint32_t element_count
        ) {
            for(uint32_t i = 0; i < element_count; ++i) {
                min_lon = std::min(
                        min_lon, std::min(objects[i].lon1, objects[i].lon2)
                );
                max_lon = std::max(
                        max_lon, std::max(objects[i].lon1, objects[i].lon2)
                );

                min_lat = std::min(
                        min_lat, std::min(objects[i].lat1, objects[i].lat2)
                );
                max_lat = std::max(
                        max_lat, std::max(objects[i].lat1, objects[i].lat2)
                );
            }
        }

        inline void AugmentMBRectangle(const RectangleInt2D & other) {
            min_lon = std::min(min_lon, other.min_lon);
            max_lon = std::max(max_lon, other.max_lon);
            min_lat = std::min(min_lat, other.min_lat);
            max_lat = std::max(max_lat, other.max_lat);
        }

        inline FixedPointCoordinate Centroid() const {
            FixedPointCoordinate centroid;
            //The coordinates of the midpoints are given by:
            //x = (x1 + x2) /2 and y = (y1 + y2) /2.
            centroid.lon = (min_lon + max_lon)/2;
            centroid.lat = (min_lat + max_lat)/2;
            return centroid;
        }

        inline bool Intersects(const RectangleInt2D & other) const {
            FixedPointCoordinate upper_left (other.max_lat, other.min_lon);
            FixedPointCoordinate upper_right(other.max_lat, other.max_lon);
            FixedPointCoordinate lower_right(other.min_lat, other.max_lon);
            FixedPointCoordinate lower_left (other.min_lat, other.min_lon);

            return (
                    Contains(upper_left)
                    || Contains(upper_right)
                    || Contains(lower_right)
                    || Contains(lower_left)
            );
        }

        inline double GetMinDist(const FixedPointCoordinate & location) const {
            bool is_contained = Contains(location);
            if (is_contained) {
                return 0.0;
            }

            double min_dist = DBL_MAX;
            min_dist = std::min(
                    min_dist,
                    ApproximateDistance(
                            location.lat,
                            location.lon,
                            max_lat,
                            min_lon
                    )
            );
            min_dist = std::min(
                    min_dist,
                    ApproximateDistance(
                            location.lat,
                            location.lon,
                            max_lat,
                            max_lon
                    )
            );
            min_dist = std::min(
                    min_dist,
                    ApproximateDistance(
                            location.lat,
                            location.lon,
                            min_lat,
                            max_lon
                    )
            );
            min_dist = std::min(
                    min_dist,
                    ApproximateDistance(
                            location.lat,
                            location.lon,
                            min_lat,
                            min_lon
                    )
            );
            return min_dist;
        }

        inline double GetMinMaxDist(const FixedPointCoordinate & location) const {
            double min_max_dist = DBL_MAX;
            //Get minmax distance to each of the four sides
            FixedPointCoordinate upper_left (max_lat, min_lon);
            FixedPointCoordinate upper_right(max_lat, max_lon);
            FixedPointCoordinate lower_right(min_lat, max_lon);
            FixedPointCoordinate lower_left (min_lat, min_lon);

            min_max_dist = std::min(
                    min_max_dist,
                    std::max(
                            ApproximateDistance(location, upper_left ),
                            ApproximateDistance(location, upper_right)
                    )
            );

            min_max_dist = std::min(
                    min_max_dist,
                    std::max(
                            ApproximateDistance(location, upper_right),
                            ApproximateDistance(location, lower_right)
                    )
            );

            min_max_dist = std::min(
                    min_max_dist,
                    std::max(
                            ApproximateDistance(location, lower_right),
                            ApproximateDistance(location, lower_left )
                    )
            );

            min_max_dist = std::min(
                    min_max_dist,
                    std::max(
                            ApproximateDistance(location, lower_left ),
                            ApproximateDistance(location, upper_left )
                    )
            );
            return min_max_dist;
        }

        inline bool Contains(const FixedPointCoordinate & location) const {
            bool lats_contained =
                    (location.lat > min_lat) && (location.lat < max_lat);
            bool lons_contained =
                    (location.lon > min_lon) && (location.lon < max_lon);
            return lats_contained && lons_contained;
        }

        inline friend std::ostream & operator<< (
            std::ostream & out,
            const RectangleInt2D & rect
        ) {
            out << rect.min_lat/COORDINATE_PRECISION << ","
                << rect.min_lon/COORDINATE_PRECISION << " "
                << rect.max_lat/COORDINATE_PRECISION << ","
                << rect.max_lon/COORDINATE_PRECISION;
            return out;
        }
    };

    typedef RectangleInt2D RectangleT;

    struct WrappedInputElement {
        explicit WrappedInputElement(
            const uint32_t _array_index,
            const uint64_t _hilbert_value
            ) : m_array_index(_array_index), m_hilbert_value(_hilbert_value) {}

        WrappedInputElement() : m_array_index(UINT_MAX), m_hilbert_value(0) {}

        uint32_t m_array_index;
        uint64_t m_hilbert_value;

        inline bool operator<(const WrappedInputElement & other) const {
            return m_hilbert_value < other.m_hilbert_value;
        }
    };

    struct LeafNode {
        LeafNode() : object_count(0) {}
        uint32_t object_count;
        DataT objects[RTREE_LEAF_NODE_SIZE];
    };

    struct TreeNode {
        TreeNode() : child_count(0), child_is_on_disk(false) {}
        RectangleT minimum_bounding_rectangle;
        uint32_t child_count:31;
        bool child_is_on_disk:1;
        uint32_t children[RTREE_BRANCHING_FACTOR];
    };

    struct QueryCandidate {
        explicit QueryCandidate(
            const uint32_t n_id,
            const double dist
        ) : node_id(n_id), min_dist(dist) {}
        QueryCandidate() : node_id(UINT_MAX), min_dist(DBL_MAX) {}
        uint32_t node_id;
        double min_dist;
        inline bool operator<(const QueryCandidate & other) const {
            return min_dist < other.min_dist;
        }
    };

    std::vector<TreeNode> m_search_tree;
    uint64_t m_element_count;

    const std::string m_leaf_node_filename;
public:
    //Construct a packed Hilbert-R-Tree with Kamel-Faloutsos algorithm [1]
    explicit StaticRTree(
        std::vector<DataT> & input_data_vector,
        const std::string tree_node_filename,
        const std::string leaf_node_filename
    )
     :  m_element_count(input_data_vector.size()),
        m_leaf_node_filename(leaf_node_filename)
    {
        SimpleLogger().Write() <<
            "constructing r-tree of " << m_element_count <<
            " elements";

        double time1 = get_timestamp();
        std::vector<WrappedInputElement> input_wrapper_vector(m_element_count);

        //generate auxiliary vector of hilbert-values
#pragma omp parallel for schedule(guided)
        for(uint64_t element_counter = 0; element_counter < m_element_count; ++element_counter) {
            input_wrapper_vector[element_counter].m_array_index = element_counter;
            //Get Hilbert-Value for centroid in mercartor projection
            DataT & current_element = input_data_vector[element_counter];
            FixedPointCoordinate current_centroid = current_element.Centroid();
            current_centroid.lat = COORDINATE_PRECISION*lat2y(current_centroid.lat/COORDINATE_PRECISION);

            uint64_t current_hilbert_value = HilbertCode::GetHilbertNumberForCoordinate(current_centroid);
            input_wrapper_vector[element_counter].m_hilbert_value = current_hilbert_value;

        }
        //open leaf file
        boost::filesystem::ofstream leaf_node_file(leaf_node_filename, std::ios::binary);
        leaf_node_file.write((char*) &m_element_count, sizeof(uint64_t));

        //sort the hilbert-value representatives
        std::sort(input_wrapper_vector.begin(), input_wrapper_vector.end());
        std::vector<TreeNode> tree_nodes_in_level;

        //pack M elements into leaf node and write to leaf file
        uint64_t processed_objects_count = 0;
        while(processed_objects_count < m_element_count) {

            LeafNode current_leaf;
            TreeNode current_node;
            for(uint32_t current_element_index = 0; RTREE_LEAF_NODE_SIZE > current_element_index; ++current_element_index) {
                if(m_element_count > (processed_objects_count + current_element_index)) {
                    uint32_t index_of_next_object = input_wrapper_vector[processed_objects_count + current_element_index].m_array_index;
                    current_leaf.objects[current_element_index] = input_data_vector[index_of_next_object];
                    ++current_leaf.object_count;
                }
            }

            //generate tree node that resemble the objects in leaf and store it for next level
            current_node.minimum_bounding_rectangle.InitializeMBRectangle(current_leaf.objects, current_leaf.object_count);
            current_node.child_is_on_disk = true;
            current_node.children[0] = tree_nodes_in_level.size();
            tree_nodes_in_level.push_back(current_node);

            //write leaf_node to leaf node file
            leaf_node_file.write((char*)&current_leaf, sizeof(current_leaf));
            processed_objects_count += current_leaf.object_count;
        }

        //close leaf file
        leaf_node_file.close();

        uint32_t processing_level = 0;
        while(1 < tree_nodes_in_level.size()) {
            std::vector<TreeNode> tree_nodes_in_next_level;
            uint32_t processed_tree_nodes_in_level = 0;
            while(processed_tree_nodes_in_level < tree_nodes_in_level.size()) {
                TreeNode parent_node;
                //pack RTREE_BRANCHING_FACTOR elements into tree_nodes each
                for(
                    uint32_t current_child_node_index = 0;
                    RTREE_BRANCHING_FACTOR > current_child_node_index;
                    ++current_child_node_index
                ) {
                    if(processed_tree_nodes_in_level < tree_nodes_in_level.size()) {
                        TreeNode & current_child_node = tree_nodes_in_level[processed_tree_nodes_in_level];
                        //add tree node to parent entry
                        parent_node.children[current_child_node_index] = m_search_tree.size();
                        m_search_tree.push_back(current_child_node);
                        //augment MBR of parent
                        parent_node.minimum_bounding_rectangle.AugmentMBRectangle(current_child_node.minimum_bounding_rectangle);
                        //increase counters
                        ++parent_node.child_count;
                        ++processed_tree_nodes_in_level;
                    }
                }
                tree_nodes_in_next_level.push_back(parent_node);
            }
            tree_nodes_in_level.swap(tree_nodes_in_next_level);
            ++processing_level;
        }
        BOOST_ASSERT_MSG(1 == tree_nodes_in_level.size(), "tree broken, more than one root node");
        //last remaining entry is the root node, store it
        m_search_tree.push_back(tree_nodes_in_level[0]);

        //reverse and renumber tree to have root at index 0
        std::reverse(m_search_tree.begin(), m_search_tree.end());
#pragma omp parallel for schedule(guided)
        for(uint32_t i = 0; i < m_search_tree.size(); ++i) {
            TreeNode & current_tree_node = m_search_tree[i];
            for(uint32_t j = 0; j < current_tree_node.child_count; ++j) {
                const uint32_t old_id = current_tree_node.children[j];
                const uint32_t new_id = m_search_tree.size() - old_id - 1;
                current_tree_node.children[j] = new_id;
            }
        }

        //open tree file
        boost::filesystem::ofstream tree_node_file(
            tree_node_filename,
            std::ios::binary
        );

        uint32_t size_of_tree = m_search_tree.size();
        BOOST_ASSERT_MSG(0 < size_of_tree, "tree empty");
        tree_node_file.write((char *)&size_of_tree, sizeof(uint32_t));
        tree_node_file.write((char *)&m_search_tree[0], sizeof(TreeNode)*size_of_tree);
        //close tree node file.
        tree_node_file.close();
        double time2 = get_timestamp();
        SimpleLogger().Write() <<
            "finished r-tree construction in " << (time2-time1) << " seconds";
    }

    //Read-only operation for queries
    explicit StaticRTree(
            const std::string & node_filename,
            const std::string & leaf_filename
    ) : m_leaf_node_filename(leaf_filename) {
        //open tree node file and load into RAM.
        boost::filesystem::path node_file(node_filename);

        if ( !boost::filesystem::exists( node_file ) ) {
            throw OSRMException("ram index file does not exist");
        }
        if ( 0 == boost::filesystem::file_size( node_file ) ) {
            throw OSRMException("ram index file is empty");
        }
        boost::filesystem::ifstream tree_node_file( node_file, std::ios::binary );

        uint32_t tree_size = 0;
        tree_node_file.read((char*)&tree_size, sizeof(uint32_t));
        //SimpleLogger().Write() << "reading " << tree_size << " tree nodes in " << (sizeof(TreeNode)*tree_size) << " bytes";
        m_search_tree.resize(tree_size);
        tree_node_file.read((char*)&m_search_tree[0], sizeof(TreeNode)*tree_size);
        tree_node_file.close();

        //open leaf node file and store thread specific pointer
        boost::filesystem::path leaf_file(leaf_filename);
        if ( !boost::filesystem::exists( leaf_file ) ) {
            throw OSRMException("mem index file does not exist");
        }
        if ( 0 == boost::filesystem::file_size( leaf_file ) ) {
            throw OSRMException("mem index file is empty");
        }

        boost::filesystem::ifstream leaf_node_file( leaf_file, std::ios::binary );
        leaf_node_file.read((char*)&m_element_count, sizeof(uint64_t));
        leaf_node_file.close();

        //SimpleLogger().Write() << tree_size << " nodes in search tree";
        //SimpleLogger().Write() << m_element_count << " elements in leafs";
    }
/*
    inline void FindKNearestPhantomNodesForCoordinate(
        const FixedPointCoordinate & location,
        const unsigned zoom_level,
        const unsigned candidate_count,
        std::vector<std::pair<PhantomNode, double> > & result_vector
    ) const {

        bool ignore_tiny_components = (zoom_level <= 14);
        DataT nearest_edge;

        uint32_t io_count = 0;
        uint32_t explored_tree_nodes_count = 0;
        SimpleLogger().Write() << "searching for coordinate " << input_coordinate;
        double min_dist = DBL_MAX;
        double min_max_dist = DBL_MAX;
        bool found_a_nearest_edge = false;

        FixedPointCoordinate nearest, current_start_coordinate, current_end_coordinate;

        //initialize queue with root element
        std::priority_queue<QueryCandidate> traversal_queue;
        traversal_queue.push(QueryCandidate(0, m_search_tree[0].minimum_bounding_rectangle.GetMinDist(input_coordinate)));
        BOOST_ASSERT_MSG(FLT_EPSILON > (0. - traversal_queue.top().min_dist), "Root element in NN Search has min dist != 0.");

        while(!traversal_queue.empty()) {
            const QueryCandidate current_query_node = traversal_queue.top(); traversal_queue.pop();

            ++explored_tree_nodes_count;
            bool prune_downward = (current_query_node.min_dist >= min_max_dist);
            bool prune_upward    = (current_query_node.min_dist >= min_dist);
            if( !prune_downward && !prune_upward ) { //downward pruning
                TreeNode & current_tree_node = m_search_tree[current_query_node.node_id];
                if (current_tree_node.child_is_on_disk) {
                    LeafNode current_leaf_node;
                    LoadLeafFromDisk(current_tree_node.children[0], current_leaf_node);
                    ++io_count;
                    for(uint32_t i = 0; i < current_leaf_node.object_count; ++i) {
                        DataT & current_edge = current_leaf_node.objects[i];
                        if(ignore_tiny_components && current_edge.belongsToTinyComponent) {
                            continue;
                        }

                        double current_ratio = 0.;
                        double current_perpendicular_distance = ComputePerpendicularDistance(
                                                                                             input_coordinate,
                                                                                             FixedPointCoordinate(current_edge.lat1, current_edge.lon1),
                                                                                             FixedPointCoordinate(current_edge.lat2, current_edge.lon2),
                                                                                             nearest,
                                                                                             &current_ratio
                                                                                             );

                        if(
                           current_perpendicular_distance < min_dist
                           && !DoubleEpsilonCompare(
                                                    current_perpendicular_distance,
                                                    min_dist
                                                    )
                           ) { //found a new minimum
                            min_dist = current_perpendicular_distance;
                            result_phantom_node.edgeBasedNode = current_edge.id;
                            result_phantom_node.nodeBasedEdgeNameID = current_edge.nameID;
                            result_phantom_node.weight1 = current_edge.weight;
                            result_phantom_node.weight2 = INT_MAX;
                            result_phantom_node.location = nearest;
                            current_start_coordinate.lat = current_edge.lat1;
                            current_start_coordinate.lon = current_edge.lon1;
                            current_end_coordinate.lat = current_edge.lat2;
                            current_end_coordinate.lon = current_edge.lon2;
                            nearest_edge = current_edge;
                            found_a_nearest_edge = true;
                        } else if(
                                  DoubleEpsilonCompare(current_perpendicular_distance, min_dist) &&
                                  1 == abs(current_edge.id - result_phantom_node.edgeBasedNode )
                                  && CoordinatesAreEquivalent(
                                                              current_start_coordinate,
                                                              FixedPointCoordinate(
                                                                          current_edge.lat1,
                                                                          current_edge.lon1
                                                                          ),
                                                              FixedPointCoordinate(
                                                                          current_edge.lat2,
                                                                          current_edge.lon2
                                                                          ),
                                                              current_end_coordinate
                                                              )
                                  ) {
                            result_phantom_node.edgeBasedNode = std::min(current_edge.id, result_phantom_node.edgeBasedNode);
                            result_phantom_node.weight2 = current_edge.weight;
                        }
                    }
                } else {
                    //traverse children, prune if global mindist is smaller than local one
                    for (uint32_t i = 0; i < current_tree_node.child_count; ++i) {
                        const int32_t child_id = current_tree_node.children[i];
                        TreeNode & child_tree_node = m_search_tree[child_id];
                        RectangleT & child_rectangle = child_tree_node.minimum_bounding_rectangle;
                        const double current_min_dist = child_rectangle.GetMinDist(input_coordinate);
                        const double current_min_max_dist = child_rectangle.GetMinMaxDist(input_coordinate);
                        if( current_min_max_dist < min_max_dist ) {
                            min_max_dist = current_min_max_dist;
                        }
                        if (current_min_dist > min_max_dist) {
                            continue;
                        }
                        if (current_min_dist > min_dist) { //upward pruning
                            continue;
                        }
                        traversal_queue.push(QueryCandidate(child_id, current_min_dist));
                    }
                }
            }
        }

        const double distance_to_edge =
        ApproximateDistance (
            FixedPointCoordinate(nearest_edge.lat1, nearest_edge.lon1),
            result_phantom_node.location
        );

        const double length_of_edge =
        ApproximateDistance(
            FixedPointCoordinate(nearest_edge.lat1, nearest_edge.lon1),
            FixedPointCoordinate(nearest_edge.lat2, nearest_edge.lon2)
        );

        const double ratio = (found_a_nearest_edge ?
          std::min(1., distance_to_edge/ length_of_edge ) : 0 );
        result_phantom_node.weight1 *= ratio;
        if(INT_MAX != result_phantom_node.weight2) {
            result_phantom_node.weight2 *= (1.-ratio);
        }
        result_phantom_node.ratio = ratio;

        //Hack to fix rounding errors and wandering via nodes.
        if(std::abs(input_coordinate.lon - result_phantom_node.location.lon) == 1) {
            result_phantom_node.location.lon = input_coordinate.lon;
        }
        if(std::abs(input_coordinate.lat - result_phantom_node.location.lat) == 1) {
            result_phantom_node.location.lat = input_coordinate.lat;
        }

        SimpleLogger().Write() << "mindist: " << min_distphantom_node.isBidirected() ? "yes" : "no");
        return found_a_nearest_edge;

    }

  */
    bool LocateClosestEndPointForCoordinate(
            const FixedPointCoordinate & input_coordinate,
            FixedPointCoordinate & result_coordinate,
            const unsigned zoom_level
    ) {
        bool ignore_tiny_components = (zoom_level <= 14);
        DataT nearest_edge;
        double min_dist = DBL_MAX;
        double min_max_dist = DBL_MAX;
        bool found_a_nearest_edge = false;

        //initialize queue with root element
        std::priority_queue<QueryCandidate> traversal_queue;
        double current_min_dist = m_search_tree[0].minimum_bounding_rectangle.GetMinDist(input_coordinate);
        traversal_queue.push(
            QueryCandidate(0, current_min_dist)
        );

        BOOST_ASSERT_MSG(
            FLT_EPSILON > (0. - traversal_queue.top().min_dist),
            "Root element in NN Search has min dist != 0."
        );

        while(!traversal_queue.empty()) {
            const QueryCandidate current_query_node = traversal_queue.top();
            traversal_queue.pop();

            const bool prune_downward = (current_query_node.min_dist >= min_max_dist);
            const bool prune_upward = (current_query_node.min_dist >= min_dist);
            if( !prune_downward && !prune_upward ) { //downward pruning
                TreeNode & current_tree_node = m_search_tree[current_query_node.node_id];
                if (current_tree_node.child_is_on_disk) {
                    LeafNode current_leaf_node;
                    LoadLeafFromDisk(
                        current_tree_node.children[0],
                        current_leaf_node
                    );
                    for(uint32_t i = 0; i < current_leaf_node.object_count; ++i) {
                        const DataT & current_edge = current_leaf_node.objects[i];
                        if(
                            ignore_tiny_components &&
                            current_edge.belongsToTinyComponent
                        ) {
                            continue;
                        }
                        if(current_edge.isIgnored()) {
                            continue;
                        }

                        double current_minimum_distance = ApproximateDistance(
                                input_coordinate.lat,
                                input_coordinate.lon,
                                current_edge.lat1,
                                current_edge.lon1
                            );
                        if( current_minimum_distance < min_dist ) {
                            //found a new minimum
                            min_dist = current_minimum_distance;
                            result_coordinate.lat = current_edge.lat1;
                            result_coordinate.lon = current_edge.lon1;
                            found_a_nearest_edge = true;
                        }

                        current_minimum_distance = ApproximateDistance(
                                input_coordinate.lat,
                                input_coordinate.lon,
                                current_edge.lat2,
                                current_edge.lon2
                            );

                        if( current_minimum_distance < min_dist ) {
                            //found a new minimum
                            min_dist = current_minimum_distance;
                            result_coordinate.lat = current_edge.lat2;
                            result_coordinate.lon = current_edge.lon2;
                            found_a_nearest_edge = true;
                        }
                    }
                } else {
                    //traverse children, prune if global mindist is smaller than local one
                    for (uint32_t i = 0; i < current_tree_node.child_count; ++i) {
                        const int32_t child_id = current_tree_node.children[i];
                        const TreeNode & child_tree_node = m_search_tree[child_id];
                        const RectangleT & child_rectangle = child_tree_node.minimum_bounding_rectangle;
                        const double current_min_dist = child_rectangle.GetMinDist(input_coordinate);
                        const double current_min_max_dist = child_rectangle.GetMinMaxDist(input_coordinate);
                        if( current_min_max_dist < min_max_dist ) {
                            min_max_dist = current_min_max_dist;
                        }
                        if (current_min_dist > min_max_dist) {
                            continue;
                        }
                        if (current_min_dist > min_dist) { //upward pruning
                            continue;
                        }
                        traversal_queue.push(
                            QueryCandidate(child_id, current_min_dist)
                        );
                    }
                }
            }
        }

        return found_a_nearest_edge;
    }


    bool FindPhantomNodeForCoordinate(
            const FixedPointCoordinate & input_coordinate,
            PhantomNode & result_phantom_node,
            const unsigned zoom_level
    ) {

        bool ignore_tiny_components = (zoom_level <= 14);
        DataT nearest_edge;

        uint32_t io_count = 0;
        uint32_t explored_tree_nodes_count = 0;
        //SimpleLogger().Write() << "searching for coordinate " << input_coordinate;
        double min_dist = DBL_MAX;
        double min_max_dist = DBL_MAX;
        bool found_a_nearest_edge = false;

        FixedPointCoordinate nearest, current_start_coordinate, current_end_coordinate;

        //initialize queue with root element
        std::priority_queue<QueryCandidate> traversal_queue;
        double current_min_dist = m_search_tree[0].minimum_bounding_rectangle.GetMinDist(input_coordinate);
        traversal_queue.push(
                             QueryCandidate(0, current_min_dist)
        );

        BOOST_ASSERT_MSG(
                         FLT_EPSILON > (0. - traversal_queue.top().min_dist),
                         "Root element in NN Search has min dist != 0."
        );

        while(!traversal_queue.empty()) {
            const QueryCandidate current_query_node = traversal_queue.top(); traversal_queue.pop();

            ++explored_tree_nodes_count;
            bool prune_downward = (current_query_node.min_dist >= min_max_dist);
            bool prune_upward    = (current_query_node.min_dist >= min_dist);
            if( !prune_downward && !prune_upward ) { //downward pruning
                TreeNode & current_tree_node = m_search_tree[current_query_node.node_id];
                if (current_tree_node.child_is_on_disk) {
                    LeafNode current_leaf_node;
                    LoadLeafFromDisk(current_tree_node.children[0], current_leaf_node);
                    ++io_count;
                    //SimpleLogger().Write() << "checking " << current_leaf_node.object_count << " elements";
                    for(uint32_t i = 0; i < current_leaf_node.object_count; ++i) {
                        DataT & current_edge = current_leaf_node.objects[i];
                        if(ignore_tiny_components && current_edge.belongsToTinyComponent) {
                            continue;
                        }
                        if(current_edge.isIgnored()) {
                            continue;
                        }

                       double current_ratio = 0.;
                       double current_perpendicular_distance = ComputePerpendicularDistance(
                                input_coordinate,
                                FixedPointCoordinate(current_edge.lat1, current_edge.lon1),
                                FixedPointCoordinate(current_edge.lat2, current_edge.lon2),
                                nearest,
                                &current_ratio
                        );

                        if(
                                current_perpendicular_distance < min_dist
                                && !DoubleEpsilonCompare(
                                        current_perpendicular_distance,
                                        min_dist
                                )
                        ) { //found a new minimum
                            min_dist = current_perpendicular_distance;
                            result_phantom_node.edgeBasedNode = current_edge.id;
                            result_phantom_node.nodeBasedEdgeNameID = current_edge.nameID;
                            result_phantom_node.weight1 = current_edge.weight;
                            result_phantom_node.weight2 = INT_MAX;
                            result_phantom_node.location = nearest;
                            current_start_coordinate.lat = current_edge.lat1;
                            current_start_coordinate.lon = current_edge.lon1;
                            current_end_coordinate.lat = current_edge.lat2;
                            current_end_coordinate.lon = current_edge.lon2;
                            nearest_edge = current_edge;
                            found_a_nearest_edge = true;
                        } else if(
                                DoubleEpsilonCompare(current_perpendicular_distance, min_dist) &&
                                1 == abs(current_edge.id - result_phantom_node.edgeBasedNode )
                        && CoordinatesAreEquivalent(
                                current_start_coordinate,
                                FixedPointCoordinate(
                                        current_edge.lat1,
                                        current_edge.lon1
                                ),
                                FixedPointCoordinate(
                                        current_edge.lat2,
                                        current_edge.lon2
                                ),
                                current_end_coordinate
                            )
                        ) {
                            BOOST_ASSERT_MSG(current_edge.id != result_phantom_node.edgeBasedNode, "IDs not different");
                            //SimpleLogger().Write() << "found bidirected edge on nodes " << current_edge.id << " and " << result_phantom_node.edgeBasedNode;
                            result_phantom_node.weight2 = current_edge.weight;
                            if(current_edge.id < result_phantom_node.edgeBasedNode) {
                                result_phantom_node.edgeBasedNode = current_edge.id;
                                std::swap(result_phantom_node.weight1, result_phantom_node.weight2);
                                std::swap(current_end_coordinate, current_start_coordinate);
                            //    SimpleLogger().Write() <<"case 2";
                            }
                            //SimpleLogger().Write() << "w1: " << result_phantom_node.weight1 << ", w2: " << result_phantom_node.weight2;
                        }
                    }
                } else {
                    //traverse children, prune if global mindist is smaller than local one
                    for (uint32_t i = 0; i < current_tree_node.child_count; ++i) {
                        const int32_t child_id = current_tree_node.children[i];
                        TreeNode & child_tree_node = m_search_tree[child_id];
                        RectangleT & child_rectangle = child_tree_node.minimum_bounding_rectangle;
                        const double current_min_dist = child_rectangle.GetMinDist(input_coordinate);
                        const double current_min_max_dist = child_rectangle.GetMinMaxDist(input_coordinate);
                        if( current_min_max_dist < min_max_dist ) {
                            min_max_dist = current_min_max_dist;
                        }
                        if (current_min_dist > min_max_dist) {
                            continue;
                        }
                        if (current_min_dist > min_dist) { //upward pruning
                            continue;
                        }
                        traversal_queue.push(QueryCandidate(child_id, current_min_dist));
                    }
                }
            }
        }

        const double ratio = (found_a_nearest_edge ?
            std::min(1., ApproximateDistance(current_start_coordinate,
                result_phantom_node.location)/ApproximateDistance(current_start_coordinate, current_end_coordinate)
                ) : 0
            );
        result_phantom_node.weight1 *= ratio;
        if(INT_MAX != result_phantom_node.weight2) {
            result_phantom_node.weight2 *= (1.-ratio);
        }
        result_phantom_node.ratio = ratio;

        //Hack to fix rounding errors and wandering via nodes.
        if(std::abs(input_coordinate.lon - result_phantom_node.location.lon) == 1) {
            result_phantom_node.location.lon = input_coordinate.lon;
        }
        if(std::abs(input_coordinate.lat - result_phantom_node.location.lat) == 1) {
            result_phantom_node.location.lat = input_coordinate.lat;
        }

        return found_a_nearest_edge;

    }

private:
    inline void LoadLeafFromDisk(const uint32_t leaf_id, LeafNode& result_node) {
        if(
            !thread_local_rtree_stream.get() ||
            !thread_local_rtree_stream->is_open()
        ) {
            thread_local_rtree_stream.reset(
                new boost::filesystem::ifstream(
                        m_leaf_node_filename,
                        std::ios::in | std::ios::binary
                )
            );
        }
        if(!thread_local_rtree_stream->good()) {
            thread_local_rtree_stream->clear(std::ios::goodbit);
            SimpleLogger().Write(logDEBUG) << "Resetting stale filestream";
        }
        uint64_t seek_pos = sizeof(uint64_t) + leaf_id*sizeof(LeafNode);
        thread_local_rtree_stream->seekg(seek_pos);
        thread_local_rtree_stream->read((char *)&result_node, sizeof(LeafNode));
    }

    inline double ComputePerpendicularDistance(
            const FixedPointCoordinate& inputPoint,
            const FixedPointCoordinate& source,
            const FixedPointCoordinate& target,
            FixedPointCoordinate& nearest, double *r) const {
        const double x = static_cast<double>(inputPoint.lat);
        const double y = static_cast<double>(inputPoint.lon);
        const double a = static_cast<double>(source.lat);
        const double b = static_cast<double>(source.lon);
        const double c = static_cast<double>(target.lat);
        const double d = static_cast<double>(target.lon);
        double p,q,mX,nY;
        if(fabs(a-c) > FLT_EPSILON){
            const double m = (d-b)/(c-a); // slope
            // Projection of (x,y) on line joining (a,b) and (c,d)
            p = ((x + (m*y)) + (m*m*a - m*b))/(1. + m*m);
            q = b + m*(p - a);
        } else {
            p = c;
            q = y;
        }
        nY = (d*p - c*q)/(a*d - b*c);
        mX = (p - nY*a)/c;// These values are actually n/m+n and m/m+n , we need
        // not calculate the explicit values of m an n as we
        // are just interested in the ratio
        if(std::isnan(mX)) {
            *r = (target == inputPoint) ? 1. : 0.;
        } else {
            *r = mX;
        }
        if(*r<=0.){
            nearest.lat = source.lat;
            nearest.lon = source.lon;
            return ((b - y)*(b - y) + (a - x)*(a - x));
//            return std::sqrt(((b - y)*(b - y) + (a - x)*(a - x)));
        } else if(*r >= 1.){
            nearest.lat = target.lat;
            nearest.lon = target.lon;
            return ((d - y)*(d - y) + (c - x)*(c - x));
//            return std::sqrt(((d - y)*(d - y) + (c - x)*(c - x)));
        }
        // point lies in between
        nearest.lat = p;
        nearest.lon = q;
//        return std::sqrt((p-x)*(p-x) + (q-y)*(q-y));
        return (p-x)*(p-x) + (q-y)*(q-y);
    }

    inline bool CoordinatesAreEquivalent(const FixedPointCoordinate & a, const FixedPointCoordinate & b, const FixedPointCoordinate & c, const FixedPointCoordinate & d) const {
        return (a == b && c == d) || (a == c && b == d) || (a == d && b == c);
    }

    inline bool DoubleEpsilonCompare(const double d1, const double d2) const {
        return (std::fabs(d1 - d2) < FLT_EPSILON);
    }

};

//[1] "On Packing R-Trees"; I. Kamel, C. Faloutsos; 1993; DOI: 10.1145/170088.170403
//[2] "Nearest Neighbor Queries", N. Roussopulos et al; 1995; DOI: 10.1145/223784.223794


#endif /* STATICRTREE_H_ */
