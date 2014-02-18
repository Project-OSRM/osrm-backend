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

#include "GeometryCompressor.h"
#include "../Util/SimpleLogger.h"

#include <boost/assert.hpp>
#include <boost/foreach.hpp>

#include <fstream>

int current_free_list_maximum = 0;
int UniqueNumber () { return ++current_free_list_maximum; }

GeometryCompressor::GeometryCompressor() {
    m_free_list.reserve(100);
    IncreaseFreeList();
}

void GeometryCompressor::IncreaseFreeList() {
    m_compressed_geometries.resize(m_compressed_geometries.size() + 100);
    for(unsigned i = 100; i > 0; --i) {
        m_free_list.push_back(current_free_list_maximum);
        ++current_free_list_maximum;
    }
}

bool GeometryCompressor::HasEntryForID(const EdgeID edge_id) const {
    return (m_edge_id_to_list_index_map.find(edge_id) != m_edge_id_to_list_index_map.end());
}

unsigned GeometryCompressor::GetPositionForID(const EdgeID edge_id) const {
    boost::unordered_map<EdgeID, unsigned>::const_iterator map_iterator;
    map_iterator = m_edge_id_to_list_index_map.find(edge_id);
    BOOST_ASSERT( map_iterator != m_edge_id_to_list_index_map.end() );
    BOOST_ASSERT( map_iterator->second < m_compressed_geometries.size() );
    return map_iterator->second;
}

void GeometryCompressor::AddLastViaNodeIDToCompressedEdge(
    const EdgeID edge_id,
    const NodeID node_id,
    const EdgeWeight weight
) {
    unsigned index = GetPositionForID(edge_id);
    BOOST_ASSERT( index < m_compressed_geometries.size() );
    if( !m_compressed_geometries[index].empty() ) {
        if( m_compressed_geometries[index].back().first == node_id ) {
            return;
        }
    }
    BOOST_ASSERT( node_id != m_compressed_geometries[index].back().first );
    m_compressed_geometries[index].push_back( std::make_pair(node_id, weight) );
    BOOST_ASSERT( node_id == m_compressed_geometries[index].back().first );
}

void GeometryCompressor::SerializeInternalVector(
    const std::string & path
) const {
    //TODO: remove super-trivial geometries

    std::ofstream geometry_out_stream( path.c_str(), std::ios::binary );
    const unsigned number_of_compressed_geometries = m_compressed_geometries.size()+1;
    BOOST_ASSERT( UINT_MAX != number_of_compressed_geometries );
    geometry_out_stream.write(
        (char*)&number_of_compressed_geometries,
        sizeof(unsigned)
    );

    SimpleLogger().Write(logDEBUG) << "number_of_compressed_geometries: " << number_of_compressed_geometries;

    // write indices array
    unsigned prefix_sum_of_list_indices = 0;
    for(unsigned i = 0; i < m_compressed_geometries.size(); ++i ) {
        geometry_out_stream.write(
            (char*)&prefix_sum_of_list_indices,
            sizeof(unsigned)
        );

        const std::vector<CompressedNode> & current_vector = m_compressed_geometries.at(i);
        const unsigned unpacked_size = current_vector.size();
        BOOST_ASSERT( UINT_MAX != unpacked_size );
        prefix_sum_of_list_indices += unpacked_size;
    }
    // sentinel element
    geometry_out_stream.write(
        (char*)&prefix_sum_of_list_indices,
        sizeof(unsigned)
    );

    // number of geometry entries to follow, it is the (inclusive) prefix sum
    geometry_out_stream.write(
        (char*)&prefix_sum_of_list_indices,
        sizeof(unsigned)
    );

    SimpleLogger().Write(logDEBUG) << "number of geometry nodes: " << prefix_sum_of_list_indices;
    unsigned control_sum = 0;
    // write compressed geometries
    for(unsigned i = 0; i < m_compressed_geometries.size(); ++i ) {
        const std::vector<CompressedNode> & current_vector = m_compressed_geometries[i];
        const unsigned unpacked_size = current_vector.size();
        control_sum += unpacked_size;
        BOOST_ASSERT( UINT_MAX != unpacked_size );
        BOOST_FOREACH(const CompressedNode current_node, current_vector ) {
            geometry_out_stream.write(
                (char*)&(current_node.first),
                sizeof(NodeID)
            );
        }
    }
    BOOST_ASSERT( control_sum == prefix_sum_of_list_indices );
    // all done, let's close the resource
    geometry_out_stream.close();
}

void GeometryCompressor::CompressEdge(
    const EdgeID surviving_edge_id,
    const EdgeID removed_edge_id,
    const NodeID via_node_id,
    const EdgeWeight weight1//,
    // const EdgeWeight weight2
) {

    BOOST_ASSERT( SPECIAL_EDGEID != surviving_edge_id );
    BOOST_ASSERT( SPECIAL_NODEID != removed_edge_id   );
    BOOST_ASSERT( SPECIAL_NODEID != via_node_id       );
    BOOST_ASSERT( std::numeric_limits<unsigned>::max() != weight1 );
    // append list of removed edge_id plus via node to surviving edge id:
    // <surv_1, .. , surv_n, via_node_id, rem_1, .. rem_n
    //
    // General scheme:
    // 1. append via node id to list of surviving_edge_id
    // 2. find list for removed_edge_id, if yes add all elements and delete it

    // Add via node id. List is created if it does not exist
    if(
        m_edge_id_to_list_index_map.find(surviving_edge_id) == m_edge_id_to_list_index_map.end()
    ) {
        // create a new entry in the map
        if( 0 == m_free_list.size() ) {
            // make sure there is a place to put the entries
            // SimpleLogger().Write() << "increased free list";
            IncreaseFreeList();
        }
        BOOST_ASSERT( !m_free_list.empty() );
        // SimpleLogger().Write() << "free list size: " << m_free_list.size();
        m_edge_id_to_list_index_map[surviving_edge_id] = m_free_list.back();
        m_free_list.pop_back();
    }
    const unsigned surving_list_id = m_edge_id_to_list_index_map[surviving_edge_id];
    BOOST_ASSERT( surving_list_id == GetPositionForID(surviving_edge_id));

    // SimpleLogger().Write() << "surviving edge id " << surviving_edge_id << " is listed at " << surving_list_id;
    BOOST_ASSERT( surving_list_id < m_compressed_geometries.size() );

    std::vector<CompressedNode> & surviving_geometry_list = m_compressed_geometries[surving_list_id];
    BOOST_ASSERT(
        surviving_geometry_list.empty() ||
        ( via_node_id != surviving_geometry_list.back().first )
    );

    if(surviving_edge_id == 0) {
        SimpleLogger().Write(logDEBUG) << "adding via " << via_node_id << ", w: " << weight1;
    }

    surviving_geometry_list.push_back( std::make_pair(via_node_id, weight1) );
    BOOST_ASSERT( 0 < surviving_geometry_list.size() );
    BOOST_ASSERT( !surviving_geometry_list.empty() );

    // Find any existing list for removed_edge_id
    typename boost::unordered_map<EdgeID, unsigned>::const_iterator remove_list_iterator;
    remove_list_iterator = m_edge_id_to_list_index_map.find(removed_edge_id);
    if( m_edge_id_to_list_index_map.end() != remove_list_iterator ) {
        const unsigned list_to_remove_index = remove_list_iterator->second;
        BOOST_ASSERT( list_to_remove_index == GetPositionForID(removed_edge_id));
        BOOST_ASSERT( list_to_remove_index < m_compressed_geometries.size() );

        std::vector<CompressedNode> & remove_geometry_list = m_compressed_geometries[list_to_remove_index];
        if(surviving_edge_id == 0) {
            SimpleLogger().Write(logDEBUG) << "appending to list: ";
            BOOST_FOREACH(const CompressedNode & node, remove_geometry_list) {
                SimpleLogger().Write(logDEBUG) << "adding via " << node.first << ", w: " << node.second;
            }
        }


        // found an existing list, append it to the list of surviving_edge_id
        surviving_geometry_list.insert(
            surviving_geometry_list.end(),
            remove_geometry_list.begin(),
            remove_geometry_list.end()
        );

        //remove the list of removed_edge_id
        m_edge_id_to_list_index_map.erase(remove_list_iterator);
        BOOST_ASSERT( m_edge_id_to_list_index_map.end() == m_edge_id_to_list_index_map.find(removed_edge_id) );
        remove_geometry_list.clear();
        BOOST_ASSERT( 0 == remove_geometry_list.size() );
        m_free_list.push_back(list_to_remove_index);
        BOOST_ASSERT( list_to_remove_index == m_free_list.back() );
    }
}

void GeometryCompressor::PrintStatistics() const {
    unsigned number_of_compressed_geometries = 0;
    const unsigned compressed_edges = m_compressed_geometries.size();

    BOOST_ASSERT( m_compressed_geometries.size() + m_free_list.size() > 0 );

    unsigned long longest_chain_length = 0;
    BOOST_FOREACH(const std::vector<CompressedNode> & current_vector, m_compressed_geometries) {
        number_of_compressed_geometries += current_vector.size();
        longest_chain_length = std::max(longest_chain_length, current_vector.size());
    }
    BOOST_ASSERT(0 == compressed_edges % 2);
    SimpleLogger().Write() <<
        "compressed edges: " << compressed_edges <<
        ", compressed geometries: " << number_of_compressed_geometries <<
        ", longest chain length: " << longest_chain_length <<
        ", cmpr ratio: " << ((float)compressed_edges/std::max(number_of_compressed_geometries, 1u) ) <<
        ", avg chain length: " << (float)number_of_compressed_geometries/std::max(1u, compressed_edges);

    SimpleLogger().Write() <<
        "No bytes: " <<  4*compressed_edges + number_of_compressed_geometries*4 +8;
}

const std::vector<GeometryCompressor::CompressedNode> & GeometryCompressor::GetBucketReference(
    const EdgeID edge_id
) const {
    const unsigned index = m_edge_id_to_list_index_map.at( edge_id );
    return m_compressed_geometries.at( index );
}
