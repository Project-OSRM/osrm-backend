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
#include "ExtractionHelperFunctions.h"

ExtractorCallbacks::ExtractorCallbacks() {externalMemory = NULL; stringMap = NULL; }
ExtractorCallbacks::ExtractorCallbacks(ExtractionContainers * ext, StringMap * strMap) {
    externalMemory = ext;
    stringMap = strMap;
}

ExtractorCallbacks::~ExtractorCallbacks() {
}

/** warning: caller needs to take care of synchronization! */
bool ExtractorCallbacks::nodeFunction(_Node &n) {
    if(n.lat <= 85*100000 && n.lat >= -85*100000)
        externalMemory->allNodes.push_back(n);
    return true;
}

bool ExtractorCallbacks::restrictionFunction(_RawRestrictionContainer &r) {
    externalMemory->restrictionsVector.push_back(r);
    return true;
}

/** warning: caller needs to take care of synchronization! */
bool ExtractorCallbacks::wayFunction(_Way &w) {
    /*** Store name of way and split it into edge segments ***/

    if ( w.speed > 0 ) { //Only true if the way is specified by the speed profile

        //Get the unique identifier for the street name
        const StringMap::const_iterator strit = stringMap->find(w.name);
        if(strit == stringMap->end()) {
            w.nameID = externalMemory->nameVector.size();
            externalMemory->nameVector.push_back(w.name);
            stringMap->insert(StringMap::value_type(w.name, w.nameID));
        } else {
            w.nameID = strit->second;
        }

        if(fabs(-1. - w.speed) < FLT_EPSILON){
            WARN("found way with bogus speed, id: " << w.id);
            return true;
        }
        if(w.id == UINT_MAX) {
            WARN("found way with unknown type: " << w.id);
            return true;
        }

        if ( w.direction == _Way::opposite ){
            std::reverse( w.path.begin(), w.path.end() );
        }

        for(std::vector< NodeID >::size_type n = 0; n < w.path.size()-1; ++n) {
            externalMemory->allEdges.push_back(_Edge(w.path[n], w.path[n+1], w.type, w.direction, w.speed, w.nameID, w.roundabout, w.ignoreInGrid, w.isDurationSet, w.isAccessRestricted));
            externalMemory->usedNodeIDs.push_back(w.path[n]);
        }
        externalMemory->usedNodeIDs.push_back(w.path.back());

        //The following information is needed to identify start and end segments of restrictions
        externalMemory->wayStartEndVector.push_back(_WayIDStartAndEndEdge(w.id, w.path[0], w.path[1], w.path[w.path.size()-2], w.path[w.path.size()-1]));
    }
    return true;
}
