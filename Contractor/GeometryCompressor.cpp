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

int current_free_list_maximum = 0;
int UniqueNumber () { return ++current_free_list_maximum; }

GeometryCompressor::GeometryCompressor() {
    m_free_list.resize(100);
    IncreaseFreeList();
}

void GeometryCompressor::IncreaseFreeList() {
    m_compressed_geometries.resize(m_compressed_geometries.size() + 100);
    std::generate_n (m_free_list.rend(), 100, UniqueNumber);
}

void GeometryCompressor::AppendNodeIDsToGeomtry( NodeID node_id, NodeID contracted_node_id ) {
    //check if node_id already has a list
    boost::unordered_map<unsigned, unsigned>::const_iterator map_iterator;
    map_iterator = m_node_id_to_index_map.find( node_id );

    unsigned geometry_bucket_index = std::numeric_limits<unsigned>::max();
    if( m_node_id_to_index_map.end() == map_iterator ) {
        //if not, create one
        if( m_free_list.empty() ) {
            IncreaseFreeList();
        }
        geometry_bucket_index = m_free_list.back();
        m_free_list.pop_back();
    } else {
        geometry_bucket_index = map_iterator->second;
    }

    BOOST_ASSERT( std::numeric_limits<unsigned>::max() != geometry_bucket_index );
    BOOST_ASSERT( geometry_bucket_index < m_compressed_geometries.size() );

    //append contracted_node_id to m_compressed_geometries[node_id]
    m_compressed_geometries[geometry_bucket_index].push_back(contracted_node_id);

    //append m_compressed_geometries[contracted_node_id] to m_compressed_geometries[node_id]
    map_iterator = m_node_id_to_index_map.find(contracted_node_id);
    if ( m_node_id_to_index_map.end() != map_iterator) {
        const unsigned bucket_index_to_remove = map_iterator->second;
        BOOST_ASSERT( bucket_index_to_remove < m_compressed_geometries.size() );

        m_compressed_geometries[geometry_bucket_index].insert(
            m_compressed_geometries[geometry_bucket_index].end(),
            m_compressed_geometries[bucket_index_to_remove].begin(),
            m_compressed_geometries[bucket_index_to_remove].end()
        );
        //remove m_compressed_geometries[contracted_node_id], add to free list
        m_compressed_geometries[bucket_index_to_remove].clear();
        m_free_list.push_back(bucket_index_to_remove);
    }
}

void GeometryCompressor::PrintStatistics() const {
    unsigned compressed_node_count = 0;
    const unsigned surviving_node_count = m_compressed_geometries.size();

    BOOST_FOREACH(const std::vector<unsigned> & current_vector, m_compressed_geometries) {
        compressed_node_count += current_vector.size();
    }
    SimpleLogger().Write() <<
        "surv: " << surviving_node_count <<
        ", comp: " << compressed_node_count <<
        ", comp ratio: " << ((float)surviving_node_count/std::max(compressed_node_count), 1);
}
