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


#ifndef OBJECTSFORQUERYSTRUCT_H_
#define OBJECTSFORQUERYSTRUCT_H_

#include<vector>
#include<string>

#include "../DataStructures/StaticGraph.h"
#include "../Util/GraphLoader.h"


struct ObjectsForQueryStruct {
    typedef StaticGraph<EdgeData> QueryGraph;
    typedef QueryGraph::InputEdge InputEdge;

    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<std::string> * names;
    QueryGraph * graph;
    unsigned checkSum;

    ObjectsForQueryStruct(std::string hsgrPath, std::string ramIndexPath, std::string fileIndexPath, std::string nodesPath, std::string namesPath, std::string psd = "route") {
        INFO("loading graph data");
        std::ifstream hsgrInStream(hsgrPath.c_str(), ios::binary);
        //Deserialize road network graph
        std::vector< QueryGraph::_StrNode> nodeList;
        std::vector< QueryGraph::_StrEdge> edgeList;
        const int n = readHSGRFromStream(hsgrInStream, nodeList, edgeList, &checkSum);
        INFO("Data checksum is " << checkSum);
        graph = new QueryGraph(nodeList, edgeList);
        assert(0 == nodeList.size());
        assert(0 == edgeList.size());
        INFO("Loading nearest neighbor indices");
        //Init nearest neighbor data structure
        std::ifstream nodesInStream(nodesPath.c_str(), ios::binary);
        nodeHelpDesk = new NodeInformationHelpDesk(ramIndexPath.c_str(), fileIndexPath.c_str(), n, checkSum);
        nodeHelpDesk->initNNGrid(nodesInStream);

        //deserialize street name list
        INFO("Loading names index");
        std::ifstream namesInStream(namesPath.c_str(), ios::binary);
        unsigned size(0);
        namesInStream.read((char *)&size, sizeof(unsigned));
        names = new std::vector<std::string>();

        char buf[1024];
        for(unsigned i = 0; i < size; ++i) {
            unsigned sizeOfString = 0;
            namesInStream.read((char *)&sizeOfString, sizeof(unsigned));
            memset(buf, 0, 1024*sizeof(char));
            namesInStream.read(buf, sizeOfString);
            std::string currentStreetName(buf);
            names->push_back(currentStreetName);
        }
        hsgrInStream.close();
        namesInStream.close();
        INFO("All query data structures loaded");
    }

    ~ObjectsForQueryStruct() {
        delete names;
        delete graph;
        delete nodeHelpDesk;
    }
};

#endif /* OBJECTSFORQUERYSTRUCT_H_ */
