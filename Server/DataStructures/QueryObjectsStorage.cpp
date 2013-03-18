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
#include "../../Util/GraphLoader.h"
#include "../../DataStructures/QueryEdge.h"
#include "../../DataStructures/StaticGraph.h"

QueryObjectsStorage::QueryObjectsStorage(std::string hsgrPath, std::string ramIndexPath, std::string fileIndexPath, std::string nodesPath, std::string edgesPath, std::string namesPath, std::string timestampPath) {
	INFO("loading graph data");
	std::ifstream hsgrInStream(hsgrPath.c_str(), std::ios::binary);
    if(!hsgrInStream) { ERR(hsgrPath <<  " not found"); }
	//Deserialize road network graph
	std::vector< QueryGraph::_StrNode> nodeList;
	std::vector< QueryGraph::_StrEdge> edgeList;
	const int n = readHSGRFromStream(hsgrInStream, nodeList, edgeList, &checkSum);

	INFO("Data checksum is " << checkSum);
	graph = new QueryGraph(nodeList, edgeList);
	assert(0 == nodeList.size());
	assert(0 == edgeList.size());

	if(timestampPath.length()) {
	    INFO("Loading Timestamp");
	    std::ifstream timestampInStream(timestampPath.c_str());
	    if(!timestampInStream) { ERR(timestampPath <<  " not found"); }

	    getline(timestampInStream, timestamp);
	    timestampInStream.close();
	}
	if(!timestamp.length())
	    timestamp = "n/a";
	if(25 < timestamp.length())
	    timestamp.resize(25);

    INFO("Loading auxiliary information");
    //Init nearest neighbor data structure
	std::ifstream nodesInStream(nodesPath.c_str(), std::ios::binary);
	if(!nodesInStream) { ERR(nodesPath <<  " not found"); }
    std::ifstream edgesInStream(edgesPath.c_str(), std::ios::binary);
    if(!edgesInStream) { ERR(edgesPath <<  " not found"); }
	nodeHelpDesk = new NodeInformationHelpDesk(ramIndexPath.c_str(), fileIndexPath.c_str(), n, checkSum, graph);
	nodeHelpDesk->initNNGrid(nodesInStream, edgesInStream);

	//deserialize street name list
	INFO("Loading names index");
	std::ifstream namesInStream(namesPath.c_str(), std::ios::binary);
    if(!namesInStream) { ERR(namesPath <<  " not found"); }
	unsigned size(0);
	namesInStream.read((char *)&size, sizeof(unsigned));
	//        names = new std::vector<std::string>();

	char buf[1024];
	for(unsigned i = 0; i < size; ++i) {
		unsigned sizeOfString = 0;
		namesInStream.read((char *)&sizeOfString, sizeof(unsigned));
		buf[sizeOfString] = '\0'; // instead of memset
		namesInStream.read(buf, sizeOfString);
		names.push_back(buf);
	}
	std::vector<std::string>(names).swap(names);
	hsgrInStream.close();
	namesInStream.close();
	INFO("All query data structures loaded");
}

QueryObjectsStorage::~QueryObjectsStorage() {
	//        delete names;
	delete graph;
	delete nodeHelpDesk;
}
