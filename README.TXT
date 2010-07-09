Compilation
---

Compiling the source code is easy. If you are running a decent linux 
installing dependencies and running make should suffic. Make sure the following
dependencies are installed:

 - Boost		1.37+
 - kdtree++	    0.7+ (0.62 does not work with g++ 4.0+)
 - sparsehash   1.4+
 - g++			4.4+
 - libxml2
 - make 

Once the dependencies are properly installed running 'make' should build the 
binaries. The Makefile has been built for Ubuntu 10.04, but it should work 
under any recent Linux. It is possible to build and run the binaries under
OS X, but for the time being manual compilation is required. Nevertheless,
the code should compile under any recent flavor of UNIX.

Running the Server
---
Running the server requires preprocessing data from Openstreetmap. To do so you
you need to acquire an .osm file. Beware of the world file, because you need
serious computing power to preprocess it. Start with a smaller country. The
preprocessing runs in three steps, all done by seperate programs.

'extractNetwork file.osm' extracts the road network of an osm file. This is
necessary, because the osm data is not made to support fast routing out of the
box. The output of the step is a file called 'file.osrm'

'createHierarchy file.osmr' preprocesses the road network and computes additional
information that is exploited later to speed up the path computation. The output
of this step consists of two file 'file.osrm.hsgr' and 'file.osrm.nodes'. The first
file is the so-called hierarchy that speeds up the path computation while the
latter one carries (among other things) geographical information.

'routed file.osmr.hsgr file.osm.hsgr' starts the server on TCP Port 5000. The
server communicates over http and can be queried by any browser or http-capable
command line tool. The server responds with KML-formatted output.Assume the 
server is installed on machine localhost and a map containing the Netherlands 
has been installed. Computing a route from Amsterdam to Den Haag can be done by
calling 
        http://localhost:5000/route&52.370197&4.890444&52.048167&4.3175
which gives a shortest (fastest) route between the two points. To locate a
point in the map that is nearest to a given coordinate use the following call
        http://localhost:5000/locate&52.048167&4.3175
