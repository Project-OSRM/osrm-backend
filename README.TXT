Compilation
---

Compiling the source code is easy. If you are running a decent linux 
installing dependencies and running make should suffice. Make sure the following
dependencies are installed (for libraries choose the -dev packages):

 - Boost	1.41+
 - g++		4.2+
 - libxml2	2.7+
 - scons 	2.10+
 - stxxl	1.3.1+
 - libprotobuf  2.3.0+ (also protobuf-compiler)
 - libbz2       any
 - libzip       any
 - libmagic++   (from ImageMagick)
  

Building the binaries is done by using scons. It should check for required
libraries and header files and report missing ones.The Scons script accepts
two optional parameters:

--cxx=XYZ lets you specify to use C++ compiler XYZ[[BR]]
--stxxlroot=/path/to/stxxl lets you specify the path to the root directory 
          of the stxxl library if it's installed in a non default location

Once the dependencies are properly installed running 'scons' should build the 
binaries. The Sconstruct has been built for Ubuntu 10.04, but it should work 
under any recent Linux. 

Compiling Under Windows
---

Visual Studio 2008 solution and projects are included for Windows compilation, and
can be found in the vsproject directory.  All required dependencies need to be
present, and the osrm.vsprops file will need to be edited to point to the various
dependency directories.  The suggested directory structure for dependencies is:

lib\
  boost\
    boost  (contains includes)
	lib
  bzip2\
    bin  (contains libbz2.dll)
	include
	lib
  iconv\
    bin
	include
	lib
  libxml2\
    bin
	include
	lib
  protobuf
    bin  (contains protoc.exe)
	include
	lib  (contains libprotobuf.lib and libprotobuf-debug.lib)
  stxxl
    include
	lib  (contains libstxxl.lib and libstxxl-debug.lib)
  zlib
    bin
	include
	lib  (zlib.lib and zlibd.lib)
	
Using the above structure, the only changes necessary for compilations are setting
the base paths in osrm.vsprops.  


Running the Server
---
Running the server requires preprocessing data from Openstreetmap. To do so you
you need to acquire an .osm file. Beware of the world file, because you need
serious computing power to preprocess it. Start with a smaller country. The
preprocessing runs in three steps, all done by seperate programs.

'osrm-extract file.osm' extracts the road network of an osm file. This is
necessary, because the osm data is not made to support fast routing out of the
box. The output of the step is a file called 'file.osrm'.

'osrm-extract' makes heavy use of STXXL memory management; STXXL will create
a temporary file in /var/tmp. If you would prefer space allocated elsewhere,
create a file named '.stxxl' in the current directory and specify the location
and size of the virtual disk there, e.g. 'disk=./stxxl,20480,syscall'.

'osrm-prepare file.osrm file.restrictions' preprocesses the road network and 
computes additional information that is exploited later to speed up the path 
computation. The output of this step consists of two file 'file.osrm.hsgr' and 
'file.osrm.nodes'. The first file is the so-called hierarchy that speeds up the 
path computation while the latter one carries (among other things) geographical
information.

'osrm-routed' starts the server on TCP Port 5000. The
server communicates over http and can be queried by any browser or http-capable
command line tool. The server responds with KML-formatted output. Assume the 
server is installed on machine localhost and a map containing the Netherlands 
has been installed. Computing a route from Amsterdam to The Hague can be done by
calling 
        http://localhost:5000/viaroute&52.370197&4.890444&52.048167&4.3175
which gives a shortest (fastest) route between the two points. To locate a
point in the map that is nearest to a given coordinate use the following call
        http://localhost:5000/nearest&52.048167&4.3175	
