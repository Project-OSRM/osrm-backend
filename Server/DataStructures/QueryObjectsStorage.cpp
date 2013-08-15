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

QueryObjectsStorage::QueryObjectsStorage(
	const std::string & hsgrPath,
	const std::string & ramIndexPath,
	const std::string & fileIndexPath,
	const std::string & nodesPath,
	const std::string & edgesPath,
	const std::string & namesPath,
	const std::string & timestampPath
) {
	if( hsgrPath.empty() ) {
		throw OSRMException("no hsgr file given in ini file");
	}
	if( ramIndexPath.empty() ) {
		throw OSRMException("no ram index file given in ini file");
	}
	if( fileIndexPath.empty() ) {
		throw OSRMException("no mem index file given in ini file");
	}
	if( nodesPath.empty() ) {
		throw OSRMException("no nodes file given in ini file");
	}
	if( edgesPath.empty() ) {
		throw OSRMException("no edges file given in ini file");
	}
	if( namesPath.empty() ) {
		throw OSRMException("no names file given in ini file");
	}

	SimpleLogger().Write() << "loading graph data";
	//Deserialize road network graph
	std::vector< QueryGraph::_StrNode> nodeList;
	std::vector< QueryGraph::_StrEdge> edgeList;
	const int n = readHSGRFromStream(
		hsgrPath,
		nodeList,
		edgeList,
		&checkSum
	);

	SimpleLogger().Write() << "Data checksum is " << checkSum;
	graph = new QueryGraph(nodeList, edgeList);
	assert(0 == nodeList.size());
	assert(0 == edgeList.size());

	if(timestampPath.length()) {
	    SimpleLogger().Write() << "Loading Timestamp";
	    std::ifstream timestampInStream(timestampPath.c_str());
	    if(!timestampInStream) {
	    	SimpleLogger().Write(logWARNING) <<  timestampPath <<  " not found";
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
		ramIndexPath,
		fileIndexPath,
		nodesPath,
		edgesPath,
		n,
		checkSum
	);

	//deserialize street name list
	SimpleLogger().Write() << "Loading names index";
	boost::filesystem::path names_file(namesPath);

    if ( !boost::filesystem::exists( names_file ) ) {
        throw OSRMException("names file does not exist");
    }
    if ( 0 == boost::filesystem::file_size( names_file ) ) {
        throw OSRMException("names file is empty");
    }

	boost::filesystem::ifstream name_stream(names_file, std::ios::binary);
	unsigned size = 0;
	name_stream.read((char *)&size, sizeof(unsigned));
	BOOST_ASSERT_MSG(0 != size, "name file empty");

	char buf[1024];
	for( unsigned i = 0; i < size; ++i ) {
		unsigned size_of_string = 0;
		name_stream.read((char *)&size_of_string, sizeof(unsigned));
		buf[size_of_string] = '\0'; // instead of memset
		name_stream.read(buf, size_of_string);
		names.push_back(buf);
	}
	std::vector<std::string>(names).swap(names);
	BOOST_ASSERT_MSG(0 != names.size(), "could not load any names");
	name_stream.close();
	SimpleLogger().Write() << "All query data structures loaded";
}

QueryObjectsStorage::~QueryObjectsStorage() {
	//        delete names;
	delete graph;
	delete nodeHelpDesk;
}
