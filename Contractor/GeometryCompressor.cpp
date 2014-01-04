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
    m_free_list.resize(100);
    IncreaseFreeList();
}

void GeometryCompressor::IncreaseFreeList() {
    m_compressed_geometries.resize(m_compressed_geometries.size() + 100);
    for(unsigned i = 100; i > 0; --i) {
        m_free_list.push_back(current_free_list_maximum);
        ++current_free_list_maximum;
    }
}

bool GeometryCompressor::HasEntryForID(const EdgeID edge_id) {
    return (m_edge_id_to_list_index_map.find(edge_id) != m_edge_id_to_list_index_map.end());
}

unsigned GeometryCompressor::GetPositionForID(const EdgeID edge_id) {
    boost::unordered_map<EdgeID, unsigned>::const_iterator map_iterator;
    map_iterator = m_edge_id_to_list_index_map.find(edge_id);
    BOOST_ASSERT( map_iterator != m_edge_id_to_list_index_map.end() );
    return map_iterator->second;
}

void GeometryCompressor::AddNodeIDToCompressedEdge(
    const EdgeID edge_id,
    const NodeID node_id
) {
    unsigned index = GetPositionForID(edge_id);
    BOOST_ASSERT( index < m_compressed_geometries.size() );
    m_compressed_geometries[index].push_back( node_id );
}

void GeometryCompressor::SerializeInternalVector(
    const std::string & path
) const {
    std::ofstream geometry_out_stream( path.c_str(), std::ios::binary );
    const unsigned compressed_edge_count = m_compressed_geometries.size()+1;
    BOOST_ASSERT( UINT_MAX != compressed_edge_count );
    geometry_out_stream.write(
        (char*)&compressed_edge_count,
        sizeof(unsigned)
    );

    // write indices array
    unsigned prefix_sum_of_list_indices = 0;
    for(unsigned i = 0; i < m_compressed_geometries.size(); ++i ) {
        const std::vector<unsigned> & current_vector = m_compressed_geometries[i];
        const unsigned unpacked_size = current_vector.size();
        BOOST_ASSERT( UINT_MAX != unpacked_size );
        geometry_out_stream.write(
            (char*)&prefix_sum_of_list_indices,
            sizeof(unsigned)
        );
        prefix_sum_of_list_indices += unpacked_size;
    }
    // write sentinel element
    geometry_out_stream.write(
        (char*)&prefix_sum_of_list_indices,
        sizeof(unsigned)
    );

    // write compressed geometries
    for(unsigned i = 0; i < m_compressed_geometries.size(); ++i ) {
        const std::vector<unsigned> & current_vector = m_compressed_geometries[i];
        const unsigned unpacked_size = current_vector.size();
        BOOST_ASSERT( UINT_MAX != unpacked_size );
        for(unsigned j = 0; j < unpacked_size; ++j) {
            geometry_out_stream.write(
                (char*)&(current_vector[j]),
                sizeof(unsigned)
            );
        }
    }

    // all done, let's close the resource
    geometry_out_stream.close();
}

void GeometryCompressor::CompressEdge(
    const EdgeID surviving_edge_id,
    const EdgeID removed_edge_id,
    const NodeID via_node_id
) {
    BOOST_ASSERT( UINT_MAX != surviving_edge_id );
    BOOST_ASSERT( UINT_MAX != removed_edge_id   );
    BOOST_ASSERT( UINT_MAX != via_node_id       );

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
            IncreaseFreeList();
        }
        m_edge_id_to_list_index_map[surviving_edge_id] = m_free_list.back();
        m_free_list.pop_back();
    }
    const unsigned surving_list_id = m_edge_id_to_list_index_map[surviving_edge_id];
    BOOST_ASSERT( surving_list_id < m_compressed_geometries.size() );

    std::vector<NodeID> & compressed_id_list = m_compressed_geometries[surving_list_id];
    compressed_id_list.push_back(via_node_id);
    BOOST_ASSERT( 0 < compressed_id_list.size() );

    // Find any existing list for removed_edge_id
    typename boost::unordered_map<EdgeID, unsigned>::const_iterator map_iterator;
    map_iterator = m_edge_id_to_list_index_map.find(removed_edge_id);
    if( m_edge_id_to_list_index_map.end() != map_iterator ) {
        const unsigned index = map_iterator->second;
        BOOST_ASSERT( index < m_compressed_geometries.size() );
        // found an existing list, append it to the list of surviving_edge_id
        compressed_id_list.insert(
            compressed_id_list.end(),
            m_compressed_geometries[index].begin(),
            m_compressed_geometries[index].end()
        );

        //remove the list of removed_edge_id
        m_edge_id_to_list_index_map.erase(map_iterator);
        BOOST_ASSERT( m_edge_id_to_list_index_map.end() == m_edge_id_to_list_index_map.find(removed_edge_id) );
        m_compressed_geometries[index].clear();
        BOOST_ASSERT( 0 == m_compressed_geometries[index].size() );
        m_free_list.push_back(index);
        BOOST_ASSERT( index == m_free_list.back() );
    }
}

void GeometryCompressor::PrintStatistics() const {
    unsigned removed_edge_count = 0;
    const unsigned surviving_edge_count = m_compressed_geometries.size()-m_free_list.size();

    BOOST_ASSERT( m_compressed_geometries.size() + m_free_list.size() > 0 );

    unsigned long longest_chain_length = 0;
    BOOST_FOREACH(const std::vector<unsigned> & current_vector, m_compressed_geometries) {
        removed_edge_count += current_vector.size();
        longest_chain_length = std::max(longest_chain_length, current_vector.size());
    }
    BOOST_ASSERT(0 == surviving_edge_count % 2);
    SimpleLogger().Write() <<
        "surviving edges: " << surviving_edge_count <<
        ", compressed edges: " << removed_edge_count <<
        ", longest chain length: " << longest_chain_length <<
        ", comp ratio: " << ((float)surviving_edge_count/std::max(removed_edge_count, 1u) ) <<
        ", avg: chain length: " << (float)removed_edge_count/std::max(1u, surviving_edge_count);

    SimpleLogger().Write() <<
        "No bytes: " <<  4*surviving_edge_count + removed_edge_count*4;
}
