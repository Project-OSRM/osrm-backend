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


#include "QueryObjectsStorage.h"

QueryObjectsStorage::QueryObjectsStorage(boost::unordered_map<const std::string,boost::filesystem::path>& paths) {
	if( paths["hsgr"].empty() ) {
		throw OSRMException("no hsgr file given in ini file");
	}
	if( paths["ramIndex"].empty() ) {
		throw OSRMException("no ram index file given in ini file");
	}
	if( paths["fileIndex"].empty() ) {
		throw OSRMException("no mem index file given in ini file");
	}
	if( paths["nodes"].empty() ) {
		throw OSRMException("no nodes file given in ini file");
	}
	if( paths["edges"].empty() ) {
		throw OSRMException("no edges file given in ini file");
	}
	if( paths["names"].empty() ) {
		throw OSRMException("no names file given in ini file");
	}

	SimpleLogger().Write() << "loading graph data";
	//Deserialize road network graph
	std::vector< QueryGraph::_StrNode> node_list;
	std::vector< QueryGraph::_StrEdge> edge_list;
	const int number_of_nodes = readHSGRFromStream(
		paths["hsgr"].c_str(),
		node_list,
		edge_list,
		&check_sum
	);

	SimpleLogger().Write() << "Data checksum is " << check_sum;
	graph = new QueryGraph(node_list, edge_list);
	assert(0 == node_list.size());
	assert(0 == edge_list.size());

	if(!paths["timestamp"].empty()) {
	    SimpleLogger().Write() << "Loading Timestamp";
	    std::ifstream timestampInStream(paths["timestamp"].c_str());
	    if(!timestampInStream) {
	        SimpleLogger().Write(logWARNING) <<  paths["timestamp"].c_str() <<  " not found";
	    }

	    getline(timestampInStream, timestamp);
	    timestampInStream.close();
	}
	if(!timestamp.length()) {
	    timestamp = "n/a";
	}
	if(25 < timestamp.length()) {
	    timestamp.resize(25);
	}

    SimpleLogger().Write() << "Loading auxiliary information";
    //Init nearest neighbor data structure
	nodeHelpDesk = new NodeInformationHelpDesk(
		paths["ramIndex"].c_str(),
		paths["fileIndex"].c_str(),
		paths["nodes"].c_str(),
		paths["edges"].c_str(),
		number_of_nodes,
		check_sum
	);

	//deserialize street name list
	SimpleLogger().Write() << "Loading names index";

    if ( !boost::filesystem::exists( paths["names"] ) ) {
        throw OSRMException("names file does not exist");
    }
    if ( 0 == boost::filesystem::file_size( paths["names"] ) ) {
        throw OSRMException("names file is empty");
    }

	boost::filesystem::ifstream name_stream(paths["names"], std::ios::binary);
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
