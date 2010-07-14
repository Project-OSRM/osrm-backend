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
#include "DataStructures/StaticGraph.h"

using namespace std;

typedef ContractionCleanup::Edge::EdgeData EdgeData;
typedef StaticGraph<EdgeData>::InputEdge GraphEdge;
typedef http::server<StaticGraph<EdgeData> > server;

/*
 * TODO: Description of command line arguments
 */

int main (int argc, char *argv[])
{
    double time;

    if(argc < 2)
    {
        cerr << "Correct usage:" << endl << argv[0] << " <hsgr data> <nodes data>" << endl;
        exit(-1);
    }

    if (argv[0] == 0) {
        cerr << "Missing data files!" << endl;
        return -1;
    }
    ifstream in(argv[1], ios::binary);
    ifstream in2(argv[2], ios::binary);
    NodeInformationHelpDesk * kdtreeService = new NodeInformationHelpDesk();

    time = get_timestamp();
    cout << "deserializing edge data from " << argv[1] << " ..." << flush;

    std::vector< GraphEdge> edgelist;
    while(!in.eof())
    {
        GraphEdge g;
        EdgeData e;

        int distance;
        bool shortcut;
        bool forward;
        bool backward;
        NodeID middle;
        NodeID source;
        NodeID target;

        in.read((char *)&(distance), sizeof(int));
        assert(distance > 0);
        in.read((char *)&(shortcut), sizeof(bool));
        in.read((char *)&(forward), sizeof(bool));
        in.read((char *)&(backward), sizeof(bool));
        in.read((char *)&(middle), sizeof(NodeID));
        in.read((char *)&(source), sizeof(NodeID));
        in.read((char *)&(target), sizeof(NodeID));
        e.backward = backward; e.distance = distance; e.forward = forward; e.middle = middle; e.shortcut = shortcut;
        g.data = e; g.source = source; g.target = target;

        edgelist.push_back(g);
    }

    in.close();
    cout << "in " << get_timestamp() - time << "s" << endl;
    cout << "search graph has " << edgelist.size() << " edges" << endl;
    time = get_timestamp();
    cout << "deserializing node map and building kd-tree ..." << flush;
    kdtreeService->initKDTree(in2);
    cout << "in " << get_timestamp() - time << "s" << endl;

    time = get_timestamp();
    cout << "building search graph ..." << flush;
    StaticGraph<EdgeData> * graph = new StaticGraph<EdgeData>(kdtreeService->getNumberOfNodes()-1, edgelist);
    cout << "checking sanity ..." << flush;
    NodeID numberOfNodes = graph->GetNumberOfNodes();
     for ( NodeID node = 0; node < numberOfNodes; ++node ) {
         for ( StaticGraph<EdgeData>::EdgeIterator edge = graph->BeginEdges( node ), endEdges = graph->EndEdges( node ); edge != endEdges; ++edge ) {
             const NodeID start = node;
             const NodeID target = graph->GetTarget( edge );
             const EdgeData& data = graph->GetEdgeData( edge );
             const NodeID middle = data.middle;
             assert(start != target);
             if(data.shortcut)
             {
                 if(graph->FindEdge(start, middle) == SPECIAL_EDGEID && graph->FindEdge(middle, start) == SPECIAL_EDGEID)
                 {
                     assert(false);
                     cerr << "hierarchy broken" << endl; exit(-1);
                 }
                 if(graph->FindEdge(middle, target) == SPECIAL_EDGEID && graph->FindEdge(target, middle) == SPECIAL_EDGEID)
                 {
                     assert(false);
                     cerr << "hierarchy broken" << endl; exit(-1);
                 }
             }
         }
     }
     cout << "ok" << endl;

    SearchEngine<EdgeData, StaticGraph<EdgeData> > * sEngine = new SearchEngine<EdgeData, StaticGraph<EdgeData> >(graph, kdtreeService);
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
    delete kdtreeService;
    return 0;
}
