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

#ifndef EXTRACTIONCONTAINERS_H_
#define EXTRACTIONCONTAINERS_H_

#include <boost/foreach.hpp>
#include <stxxl.h>

#include "ExtractorStructs.h"
#include "../DataStructures/Util.h"

class ExtractionContainers {
public:
    typedef stxxl::vector<NodeID> STXXLNodeIDVector;
    typedef stxxl::vector<_Node> STXXLNodeVector;
    typedef stxxl::vector<_Edge> STXXLEdgeVector;
    typedef stxxl::vector<std::string> STXXLStringVector;
    typedef stxxl::vector<_RawRestrictionContainer> STXXLRestrictionsVector;
    typedef stxxl::vector<_WayIDStartAndEndEdge> STXXLWayIDStartEndVector;

    ExtractionContainers() {
        //Check if another instance of stxxl is already running or if there is a general problem
        try {
            stxxl::vector<unsigned> testForRunningInstance;
        } catch(std::exception & e) {
            ERR("Could not instantiate STXXL layer." << std::endl << e.what());
        }

        nameVector.push_back("");
    }
    virtual ~ExtractionContainers() {
        usedNodeIDs.clear();
        allNodes.clear();
        allEdges.clear();
        nameVector.clear();
        restrictionsVector.clear();
        wayStartEndVector.clear();
    }

    void PrepareData( const std::string & outputFileName, const std::string restrictionsFileName, const unsigned amountOfRAM);

    STXXLNodeIDVector           usedNodeIDs;
    STXXLNodeVector             allNodes;
    STXXLEdgeVector             allEdges;
    STXXLStringVector           nameVector;
    STXXLRestrictionsVector     restrictionsVector;
    STXXLWayIDStartEndVector    wayStartEndVector;

};

#endif /* EXTRACTIONCONTAINERS_H_ */
