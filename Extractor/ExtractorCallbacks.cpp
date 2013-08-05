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


#include "ExtractorCallbacks.h"

ExtractorCallbacks::ExtractorCallbacks() {externalMemory = NULL; stringMap = NULL; }
ExtractorCallbacks::ExtractorCallbacks(ExtractionContainers * ext, StringMap * strMap) {
    externalMemory = ext;
    stringMap = strMap;
}

ExtractorCallbacks::~ExtractorCallbacks() { }

/** warning: caller needs to take care of synchronization! */
void ExtractorCallbacks::nodeFunction(const _Node &n) {
    if(n.lat <= 85*COORDINATE_PRECISION && n.lat >= -85*COORDINATE_PRECISION) {
        externalMemory->allNodes.push_back(n);
    }
}

bool ExtractorCallbacks::restrictionFunction(const _RawRestrictionContainer &r) {
    externalMemory->restrictionsVector.push_back(r);
    return true;
}

/** warning: caller needs to take care of synchronization! */
void ExtractorCallbacks::wayFunction(ExtractionWay &parsed_way) {
    if((0 < parsed_way.speed) || (0 < parsed_way.duration)) { //Only true if the way is specified by the speed profile
        if(UINT_MAX == parsed_way.id){
            DEBUG("found bogus way with id: " << parsed_way.id << " of size " << parsed_way.path.size());
            return;
        }

        if(0 < parsed_way.duration) {
         //TODO: iterate all way segments and set duration corresponding to the length of each segment
            parsed_way.speed = parsed_way.duration/(parsed_way.path.size()-1);
        }

        if(FLT_EPSILON >= fabs(-1. - parsed_way.speed)){
            DEBUG("found way with bogus speed, id: " << parsed_way.id);
            return;
        }

        //Get the unique identifier for the street name
        const StringMap::const_iterator string_map_iterator = stringMap->find(parsed_way.name);
        if(stringMap->end() == string_map_iterator) {
            parsed_way.nameID = externalMemory->nameVector.size();
            externalMemory->nameVector.push_back(parsed_way.name);
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
            externalMemory->allEdges.push_back(
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
            externalMemory->usedNodeIDs.push_back(parsed_way.path[n]);
        }
        externalMemory->usedNodeIDs.push_back(parsed_way.path.back());

        //The following information is needed to identify start and end segments of restrictions
        externalMemory->wayStartEndVector.push_back(_WayIDStartAndEndEdge(parsed_way.id, parsed_way.path[0], parsed_way.path[1], parsed_way.path[parsed_way.path.size()-2], parsed_way.path.back()));

        if(split_bidirectional_edge) { //Only true if the way should be split
            std::reverse( parsed_way.path.begin(), parsed_way.path.end() );
            for(std::vector< NodeID >::size_type n = 0; n < parsed_way.path.size()-1; ++n) {
                externalMemory->allEdges.push_back(
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
            externalMemory->wayStartEndVector.push_back(_WayIDStartAndEndEdge(parsed_way.id, parsed_way.path[0], parsed_way.path[1], parsed_way.path[parsed_way.path.size()-2], parsed_way.path.back()));
        }
    }
}
