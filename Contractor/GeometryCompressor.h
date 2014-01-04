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

#include "../typedefs.h"

#include <boost/unordered_map.hpp>

#include <limits>
#include <vector>

#ifndef GEOMETRY_COMPRESSOR_H
#define GEOMETRY_COMPRESSOR_H

class GeometryCompressor {
public:
    GeometryCompressor();
    void CompressEdge(
        const EdgeID first_edge_id,
        const EdgeID second_edge_id,
        const NodeID via_node_id
    );
    void PrintStatistics() const;
    bool HasEntryForID(const EdgeID edge_id);
    void AddNodeIDToCompressedEdge(const EdgeID edge_id, const NodeID node_id);
    unsigned GetPositionForID(const EdgeID edge_id);
    void SerializeInternalVector(const std::string & path) const;

private:

    void IncreaseFreeList();

    std::vector<std::vector<NodeID> > m_compressed_geometries;
    std::vector<unsigned> m_free_list;
    boost::unordered_map<EdgeID, unsigned> m_edge_id_to_list_index_map;
};

#endif //GEOMETRY_COMPRESSOR_H
