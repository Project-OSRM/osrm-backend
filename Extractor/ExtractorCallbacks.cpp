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

#include "ExtractorCallbacks.h"

ExtractorCallbacks::ExtractorCallbacks() {externalMemory = NULL; stringMap = NULL; }
ExtractorCallbacks::ExtractorCallbacks(ExtractionContainers * ext, StringMap * strMap) {
    externalMemory = ext;
    stringMap = strMap;
}

ExtractorCallbacks::~ExtractorCallbacks() { }

/** warning: caller needs to take care of synchronization! */
void ExtractorCallbacks::nodeFunction(const ExternalMemoryNode &n) {
    if(n.lat <= 85*COORDINATE_PRECISION && n.lat >= -85*COORDINATE_PRECISION) {
        externalMemory->all_nodes_list.push_back(n);
    }
}

bool ExtractorCallbacks::restrictionFunction(const InputRestrictionContainer &r) {
    externalMemory->restrictions_list.push_back(r);
    return true;
}

/** warning: caller needs to take care of synchronization! */
void ExtractorCallbacks::wayFunction(ExtractionWay &parsed_way) {
    if((0 < parsed_way.speed) || (0 < parsed_way.duration)) { //Only true if the way is specified by the speed profile
        if(UINT_MAX == parsed_way.id){
            SimpleLogger().Write(logDEBUG) <<
                "found bogus way with id: " << parsed_way.id <<
                " of size " << parsed_way.path.size();
            return;
        }

        if(0 < parsed_way.duration) {
         //TODO: iterate all way segments and set duration corresponding to the length of each segment
            parsed_way.speed = parsed_way.duration/(parsed_way.path.size()-1);
        }

        if(std::numeric_limits<double>::epsilon() >= fabs(-1. - parsed_way.speed)){
            SimpleLogger().Write(logDEBUG) <<
                "found way with bogus speed, id: " << parsed_way.id;
            return;
        }

        //Get the unique identifier for the street name
        const StringMap::const_iterator string_map_iterator = stringMap->find(parsed_way.name);
        if(stringMap->end() == string_map_iterator) {
            parsed_way.nameID = externalMemory->name_list.size();
            externalMemory->name_list.push_back(parsed_way.name);
            stringMap->insert(std::make_pair(parsed_way.name, parsed_way.nameID));
        } else {
            parsed_way.nameID = string_map_iterator->second;
        }

        if(ExtractionWay::opposite == parsed_way.direction) {
            std::reverse( parsed_way.path.begin(), parsed_way.path.end() );
            parsed_way.direction = ExtractionWay::oneway;
        }

        const bool split_bidirectional_edge = (parsed_way.backward_speed > 0) && (parsed_way.speed != parsed_way.backward_speed);

        for(std::vector< NodeID >::size_type n = 0; n < parsed_way.path.size()-1; ++n) {
            externalMemory->all_edges_list.push_back(
                    InternalExtractorEdge(parsed_way.path[n],
                            parsed_way.path[n+1],
                            parsed_way.type,
                            (split_bidirectional_edge ? ExtractionWay::oneway : parsed_way.direction),
                            parsed_way.speed,
                            parsed_way.nameID,
                            parsed_way.roundabout,
                            parsed_way.ignoreInGrid,
                            (0 < parsed_way.duration),
                            parsed_way.isAccessRestricted
                    )
            );
            externalMemory->used_node_id_list.push_back(parsed_way.path[n]);
        }
        externalMemory->used_node_id_list.push_back(parsed_way.path.back());

        //The following information is needed to identify start and end segments of restrictions
        externalMemory->way_start_end_id_list.push_back(_WayIDStartAndEndEdge(parsed_way.id, parsed_way.path[0], parsed_way.path[1], parsed_way.path[parsed_way.path.size()-2], parsed_way.path.back()));

        if(split_bidirectional_edge) { //Only true if the way should be split
            std::reverse( parsed_way.path.begin(), parsed_way.path.end() );
            for(std::vector< NodeID >::size_type n = 0; n < parsed_way.path.size()-1; ++n) {
                externalMemory->all_edges_list.push_back(
                        InternalExtractorEdge(parsed_way.path[n],
                                parsed_way.path[n+1],
                                parsed_way.type,
                                ExtractionWay::oneway,
                                parsed_way.backward_speed,
                                parsed_way.nameID,
                                parsed_way.roundabout,
                                parsed_way.ignoreInGrid,
                                (0 < parsed_way.duration),
                                parsed_way.isAccessRestricted,
                                (ExtractionWay::oneway == parsed_way.direction)
                        )
                );
            }
            externalMemory->way_start_end_id_list.push_back(_WayIDStartAndEndEdge(parsed_way.id, parsed_way.path[0], parsed_way.path[1], parsed_way.path[parsed_way.path.size()-2], parsed_way.path.back()));
        }
    }
}
