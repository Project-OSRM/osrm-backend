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

#ifndef QUERYOBJECTSSTORAGE_H_
#define QUERYOBJECTSSTORAGE_H_

#include "../../Util/GraphLoader.h"
#include "../../Util/OSRMException.h"
#include "../../Util/ProgramOptions.h"
#include "../../Util/SimpleLogger.h"
#include "../../DataStructures/NodeInformationHelpDesk.h"
#include "../../DataStructures/QueryEdge.h"
#include "../../DataStructures/StaticGraph.h"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <vector>
#include <string>

struct QueryObjectsStorage {
    typedef StaticGraph<QueryEdge::EdgeData>    QueryGraph;
    typedef QueryGraph::InputEdge               InputEdge;

    NodeInformationHelpDesk                   * nodeHelpDesk;
    std::vector<char>                           m_names_char_list;
    std::vector<unsigned>                       m_name_begin_indices;
    QueryGraph                                * graph;
    std::string                                 timestamp;
    unsigned                                    check_sum;

    void GetName( const unsigned name_id, std::string & result ) const;

    QueryObjectsStorage( const ServerPaths & paths );
    ~QueryObjectsStorage();
};

#endif /* QUERYOBJECTSSTORAGE_H_ */
