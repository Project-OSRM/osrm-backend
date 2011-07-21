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


#ifndef OBJECTFORPLUGINSTRUCT_H_
#define OBJECTFORPLUGINSTRUCT_H_

#include "../DataStructures/StaticGraph.h"
#include "../Util/GraphLoader.h"

typedef StaticGraph<EdgeData> QueryGraph;
typedef QueryGraph::InputEdge InputEdge;

struct ObjectsForQueryStruct {
    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<std::string> * names;
    QueryGraph * graph;

    ObjectsForQueryStruct(std::string hsgrPath, std::string ramIndexPath, std::string fileIndexPath, std::string nodesPath, std::string namesPath, std::string psd = "route") {
        std::cout << "[objects] loading query data structures ..." << std::flush;
        //Init nearest neighbor data structure
        ifstream nodesInStream(nodesPath.c_str(), ios::binary);
        nodeHelpDesk = new NodeInformationHelpDesk(ramIndexPath.c_str(), fileIndexPath.c_str());
        nodeHelpDesk->initNNGrid(nodesInStream);

        ifstream hsgrInStream(hsgrPath.c_str(), ios::binary);
        //Deserialize road network graph
        std::vector< InputEdge> edgeList;
        readHSGRFromStream(hsgrInStream, edgeList);
        graph = new QueryGraph(nodeHelpDesk->getNumberOfNodes()-1, edgeList);
        std::vector< InputEdge >().swap( edgeList ); //free memory

        //deserialize street name list
        ifstream namesInStream(namesPath.c_str(), ios::binary);
        unsigned size(0);
        namesInStream.read((char *)&size, sizeof(unsigned));
        names = new std::vector<std::string>();

        char buf[1024];
        for(unsigned i = 0; i < size; i++) {
            unsigned sizeOfString = 0;
            namesInStream.read((char *)&sizeOfString, sizeof(unsigned));
            memset(buf, 0, 1024*sizeof(char));
            namesInStream.read(buf, sizeOfString);
            std::string currentStreetName(buf);
            names->push_back(currentStreetName);
        }
        hsgrInStream.close();
        namesInStream.close();
        std::cout << "ok" << std::endl;
    }

    ~ObjectsForQueryStruct() {
        delete names;
        delete graph;
        delete nodeHelpDesk;
    }
};

#endif /* OBJECTFORPLUGINSTRUCT_H_ */
