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

#ifndef EXTRACTIONCONTAINERS_H_
#define EXTRACTIONCONTAINERS_H_

#include "ExtractorStructs.h"
#include "../Util/SimpleLogger.h"
#include "../Util/TimingUtil.h"
#include "../Util/UUID.h"

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <stxxl.h>

class ExtractionContainers {
public:
    typedef stxxl::vector<NodeID>                   STXXLNodeIDVector;
    typedef stxxl::vector<_Node>                    STXXLNodeVector;
    typedef stxxl::vector<InternalExtractorEdge>    STXXLEdgeVector;
    typedef stxxl::vector<std::string>              STXXLStringVector;
    typedef stxxl::vector<_RawRestrictionContainer> STXXLRestrictionsVector;
    typedef stxxl::vector<_WayIDStartAndEndEdge>    STXXLWayIDStartEndVector;


    STXXLNodeIDVector                               usedNodeIDs;
    STXXLNodeVector                                 allNodes;
    STXXLEdgeVector                                 allEdges;
    STXXLStringVector                               name_list;
    STXXLRestrictionsVector                         restrictionsVector;
    STXXLWayIDStartEndVector                        wayStartEndVector;
    const UUID uuid;

    ExtractionContainers() {
        //Check if another instance of stxxl is already running or if there is a general problem
        stxxl::vector<unsigned> testForRunningInstance;
        name_list.push_back("");
    }

    virtual ~ExtractionContainers() {
        usedNodeIDs.clear();
        allNodes.clear();
        allEdges.clear();
        name_list.clear();
        restrictionsVector.clear();
        wayStartEndVector.clear();
    }

    void PrepareData(
        const std::string & output_file_name,
        const std::string restrictionsFileName,
        const unsigned amountOfRAM
    );
};

#endif /* EXTRACTIONCONTAINERS_H_ */
