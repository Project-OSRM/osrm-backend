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

#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <stxxl/sort>
#include <stxxl/vector>

class ExtractionContainers {
public:
    typedef stxxl::vector<NodeID>                    STXXLNodeIDVector;
    typedef stxxl::vector<ExternalMemoryNode>        STXXLNodeVector;
    typedef stxxl::vector<InternalExtractorEdge>     STXXLEdgeVector;
    typedef stxxl::vector<std::string>               STXXLStringVector;
    typedef stxxl::vector<InputRestrictionContainer> STXXLRestrictionsVector;
    typedef stxxl::vector<_WayIDStartAndEndEdge>     STXXLWayIDStartEndVector;

    STXXLNodeIDVector                               used_node_id_list;
    STXXLNodeVector                                 all_nodes_list;
    STXXLEdgeVector                                 all_edges_list;
    STXXLStringVector                               name_list;
    STXXLRestrictionsVector                         restrictions_list;
    STXXLWayIDStartEndVector                        way_start_end_id_list;
    const UUID uuid;

    ExtractionContainers() {
        //Check if stxxl can be instantiated
        stxxl::vector<unsigned> dummy_vector;
        name_list.push_back("");
    }

    virtual ~ExtractionContainers() {
        used_node_id_list.clear();
        all_nodes_list.clear();
        all_edges_list.clear();
        name_list.clear();
        restrictions_list.clear();
        way_start_end_id_list.clear();
    }

    void PrepareData(
        const std::string & output_file_name,
        const std::string & restrictions_file_name
    );
};

#endif /* EXTRACTIONCONTAINERS_H_ */
