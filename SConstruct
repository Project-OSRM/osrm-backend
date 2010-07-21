#Sconstruct

import os
import sys

AddOption('--cxx', dest='cxx', type='string', nargs=1, action='store', metavar='STRING', help='C++ Compiler')
AddOption('--verbosity', dest='cxx', type='string', nargs=1, action='store', metavar='STRING', help='make Scons talking')
env = Environment(COMPILER = GetOption('cxx'))
if not GetOption('cxx') == '':
	env.Replace(CXX = GetOption('cxx'))
if sys.platform == 'win32':
    #SCons really wants to use Microsoft compiler :D
    print "Compiling has not been done on Windows"
    Exit(-1)
else:  #Mac OS X
    if sys.platform == 'darwin':
        print "Compiling is experimental on Mac"
        env.Append(CPPPATH = ['/opt/local/include/', '/opt/local/include/libxml2'])
	env.Append(LIBPATH = ['/opt/local/lib'])
	#env.Replace(CC = "g++-mp-4.4") 
    else:
        env.Append(CPPPATH = ['/usr/include', '/usr/include/include', '/usr/include/libxml2/'])

env.Append(CCFLAGS = ' -O3')
print "Compiling with: ", env['CXX']
conf = Configure(env)
if not conf.CheckCXX():
	print "No suitable C++ Compiler installed"
	Exit(-1)
if not conf.CheckHeader('omp.h'):
	print "Compiler does not support OpenMP. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('xml2', 'libxml/xmlreader.h', 'CXX'):
	print "libxml2 library or header not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('stxxl.h'):
	print "Could not locate stxxl header. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('google/sparse_hash_map'):
	print "Could not find Google Sparsehash library. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/asio.hpp'):
	print "boost/asio.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckLib('boost_thread'):
	if not conf.CheckLib('boost_thread-mt'):
		print "boost thread library not found. Exiting"
		Exit(-1)
if not conf.CheckCXXHeader('boost/thread.hpp'):
	print "boost thread header not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/bind.hpp'):
	print "boost/bind.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/thread.hpp'):
	print "boost/thread.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/noncopyable.hpp'):
	print "boost/noncopyable.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/shared_ptr.hpp'):
	print "boost/shared_ptr.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('kdtree++/kdtree.hpp'):
	print "kdtree++/kdtree.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckLib('stxxl'):
	print "stxxl library not found. Exiting"
	Exit(-1)

env.Append(CCFLAGS = ' -fopenmp')
env.Append(LINKFLAGS = ' -fopenmp')
env.Program("extractNetwork.cpp")
env.Program("extractLargeNetwork.cpp")	
env.Program("createHierarchy.cpp")
env.Append(CCFLAGS = ' -lboost_regex -lboost_iostreams -lboost_thread -lboost_system')
env.Append(LINKFLAGS = ' -lboost_regex -lboost_iostreams -lboost_thread -lboost_system')
env.Program("routed.cpp")
env = conf.Finish()

