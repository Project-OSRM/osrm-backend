/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

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

#include <climits>
#include <fstream>
#include <istream>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "typedefs.h"

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include "HttpServer/server.h"
#include "Contractor/GraphLoader.h"
#include "DataStructures/StaticGraph.h"

using namespace std;

typedef ContractionCleanup::Edge::EdgeData EdgeData;
typedef StaticGraph<EdgeData>::InputEdge GridEdge;
typedef http::server<StaticGraph<EdgeData> > server;

/*
 * TODO: Description of command line arguments
 */
int main (int argc, char *argv[])
{
    if(argc < 6)
    {
        cerr << "Correct usage:" << endl << argv[0] << " <hsgr data> <nodes data> <ram index> <file index> <names data>" << endl;
        exit(-1);
    }

    ifstream hsgrInStream(argv[1], ios::binary);
    ifstream nodesInStream(argv[2], ios::binary);
    NodeInformationHelpDesk * nodeInfoHelper = new NodeInformationHelpDesk(argv[3], argv[4]);

    double time = get_timestamp();
    cout << "deserializing edge data from " << argv[1] << " ..." << flush;

    std::vector< GridEdge> * edgeList = new std::vector< GridEdge>();
    readHSGRFromStream(hsgrInStream, edgeList);
    hsgrInStream.close();
    cout << "in " << get_timestamp() - time << "s" << endl;
    time = get_timestamp();
    cout << "deserializing node map and building nearest neighbor grid ..." << flush;
    nodeInfoHelper->initNNGrid(nodesInStream);
    cout << "in " << get_timestamp() - time << "s" << endl;
    StaticGraph<EdgeData> * graph = new StaticGraph<EdgeData>(nodeInfoHelper->getNumberOfNodes()-1, *edgeList);
    delete edgeList;
    time = get_timestamp();
    cout << "deserializing street names ..." << flush;
    ifstream namesInStream(argv[5], ios::binary);
    unsigned size = 0;
    namesInStream.read((char *)&size, sizeof(unsigned));
    vector<unsigned> * nameIndex = new vector<unsigned>(size, 0);
    vector<string> * names = new vector<string>();
    for(int i = 0; i<size; i++)
        namesInStream.read((char *)&(nameIndex->at(i)), sizeof(unsigned));

    for(int i = 0; i<size-1; i++){
        string tempString;
        for(int j = 0; j < nameIndex->at(i+1) - nameIndex->at(i); j++){
            char c;
            namesInStream.read(&c, sizeof(char));
            tempString.append(1, c);
        }
        names->push_back(tempString);
        tempString = "";
    }
    delete nameIndex;

    namesInStream.close();
    cout << "in " << get_timestamp() - time << "s" << endl;
    time = get_timestamp();

    cout << "constructing search graph ..." << flush;

    SearchEngine<EdgeData, StaticGraph<EdgeData> > * sEngine = new SearchEngine<EdgeData, StaticGraph<EdgeData> >(graph, nodeInfoHelper, names);
    cout << "in " << get_timestamp() - time << "s" << endl;
    time = get_timestamp();

    try {
        // Block all signals for background thread.
        sigset_t new_mask;
        sigfillset(&new_mask);
        sigset_t old_mask;
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

        cout << "starting web server ..." << flush;
        // Run server in background thread.
        server s("0.0.0.0", "5000", omp_get_num_procs(), sEngine);
        boost::thread t(boost::bind(&server::run, &s));
        cout << "ok" << endl;

        // Restore previous signals.
        pthread_sigmask(SIG_SETMASK, &old_mask, 0);

        // Wait for signal indicating time to shut down.
        sigset_t wait_mask;
        sigemptyset(&wait_mask);
        sigaddset(&wait_mask, SIGINT);
        sigaddset(&wait_mask, SIGQUIT);
        sigaddset(&wait_mask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
        int sig = 0;
        sigwait(&wait_mask, &sig);
        // Stop the server.
        s.stop();
        t.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }
    cout << "graceful shutdown after " << get_timestamp() - time << "s" << endl;
    delete names;
    delete sEngine;
    delete graph;
    delete nodeInfoHelper;
    return 0;
}
