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

#include "QueryObjectsStorage.h"

QueryObjectsStorage::QueryObjectsStorage( const ServerPaths & paths ) {
	if( paths.find("hsgrdata") == paths.end() ) {
		throw OSRMException("no hsgr file given in ini file");
	}
	if( paths.find("ramindex") == paths.end() ) {
		throw OSRMException("no ram index file given in ini file");
	}
	if( paths.find("fileindex") == paths.end() ) {
		throw OSRMException("no mem index file given in ini file");
	}
	if( paths.find("nodesdata") == paths.end() ) {
		throw OSRMException("no nodes file given in ini file");
	}
	if( paths.find("edgesdata") == paths.end() ) {
		throw OSRMException("no edges file given in ini file");
	}
	if( paths.find("namesdata") == paths.end() ) {
		throw OSRMException("no names file given in ini file");
	}

	SimpleLogger().Write() << "loading graph data";
	//Deserialize road network graph

	ServerPaths::const_iterator paths_iterator = paths.find("hsgrdata");
	BOOST_ASSERT(paths.end() != paths_iterator);
	const std::string & hsgr_data_string = paths_iterator->second.string();

	std::vector< QueryGraph::_StrNode> node_list;
	std::vector< QueryGraph::_StrEdge> edge_list;
	const int number_of_nodes = readHSGRFromStream(
		hsgr_data_string,
		node_list,
		edge_list,
		&check_sum
	);

	SimpleLogger().Write() << "Data checksum is " << check_sum;
	graph = new QueryGraph(node_list, edge_list);
	BOOST_ASSERT(0 == node_list.size());
	BOOST_ASSERT(0 == edge_list.size());

	paths_iterator = paths.find("timestamp");

	if(paths.end() != paths_iterator) {
	    SimpleLogger().Write() << "Loading Timestamp";
    	const std::string & timestamp_string = paths_iterator->second.string();

	    boost::filesystem::ifstream time_stamp_instream(timestamp_string);
	    if( !time_stamp_instream.good() ) {
	        SimpleLogger().Write(logWARNING) << timestamp_string << " not found";
	    }

	    getline(time_stamp_instream, timestamp);
	    time_stamp_instream.close();
	}
	if(!timestamp.length()) {
	    timestamp = "n/a";
	}
	if(25 < timestamp.length()) {
	    timestamp.resize(25);
	}
    SimpleLogger().Write() << "Loading auxiliary information";

    paths_iterator = paths.find("ramindex");
    BOOST_ASSERT(paths.end() != paths_iterator);
    const std::string & ram_index_string = paths_iterator->second.string();
	paths_iterator = paths.find("fileindex");
    BOOST_ASSERT(paths.end() != paths_iterator);
	const std::string & file_index_string = paths_iterator->second.string();
	paths_iterator = paths.find("nodesdata");
    BOOST_ASSERT(paths.end() != paths_iterator);
	const std::string & nodes_data_string = paths_iterator->second.string();
	paths_iterator = paths.find("edgesdata");
    BOOST_ASSERT(paths.end() != paths_iterator);
	const std::string & edges_data_string = paths_iterator->second.string();

    //Init nearest neighbor data structure
	nodeHelpDesk = new NodeInformationHelpDesk(
		ram_index_string,
		file_index_string,
		nodes_data_string,
		edges_data_string,
		number_of_nodes,
		check_sum
	);

	//deserialize street name list
	SimpleLogger().Write() << "Loading names index";

	paths_iterator = paths.find("namesdata");
    BOOST_ASSERT(paths.end() != paths_iterator);
	const std::string & names_data_string = paths_iterator->second.string();
    if ( !boost::filesystem::exists( paths_iterator->second ) ) {
        throw OSRMException("names file does not exist");
    }
    if ( 0 == boost::filesystem::file_size( paths_iterator->second ) ) {
        throw OSRMException("names file is empty");
    }

	boost::filesystem::ifstream name_stream(names_data_string, std::ios::binary);
	unsigned size = 0;
	name_stream.read((char *)&size, sizeof(unsigned));
	BOOST_ASSERT_MSG(0 != size, "name file broken");

	m_name_begin_indices.resize(size);
	name_stream.read((char*)&m_name_begin_indices[0], size*sizeof(unsigned));
	name_stream.read((char *)&size, sizeof(unsigned));
	BOOST_ASSERT_MSG(0 != size, "name file broken");

	m_names_char_list.resize(size+1); //+1 is sentinel/dummy element
	name_stream.read((char *)&m_names_char_list[0], size*sizeof(char));
	BOOST_ASSERT_MSG(0 != m_names_char_list.size(), "could not load any names");

	name_stream.close();
	SimpleLogger().Write() << "All query data structures loaded";
}

void QueryObjectsStorage::GetName(
	const unsigned name_id,
	std::string & result
) const {
	if(UINT_MAX == name_id) {
		result = "";
		return;
	}
	BOOST_ASSERT_MSG(
		name_id < m_name_begin_indices.size(),
		"name id too high"
	);
	unsigned begin_index = m_name_begin_indices[name_id];
	unsigned end_index = m_name_begin_indices[name_id+1];
	BOOST_ASSERT_MSG(
		begin_index < m_names_char_list.size(),
		"begin index of name too high"
	);
	BOOST_ASSERT_MSG(
		end_index < m_names_char_list.size(),
		"end index of name too high"
	);

	BOOST_ASSERT_MSG(begin_index <= end_index, "string ends before begin");
	result.clear();
	result.resize(end_index - begin_index);
	std::copy(
		m_names_char_list.begin() + begin_index,
		m_names_char_list.begin() + end_index,
		result.begin()
	);
}

QueryObjectsStorage::~QueryObjectsStorage() {
	delete graph;
	delete nodeHelpDesk;
}
