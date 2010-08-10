#Sconstruct

import os
import sys

AddOption('--cxx', dest='cxx', type='string', nargs=1, action='store', metavar='STRING', help='C++ Compiler')
AddOption('--stxxlroot', dest='stxxlroot', type='string', nargs=1, action='store', metavar='STRING', help='root directory of STXXL')
AddOption('--verbosity', dest='verbosity', type='string', nargs=1, action='store', metavar='STRING', help='make Scons talking')
AddOption('--buildconfiguration', dest='buildconfiguration', type='string', nargs=1, action='store', metavar='STRING', help='debug or release')
env = Environment(COMPILER = GetOption('cxx'))
if GetOption('cxx') is None:
    #default Compiler
    print 'Using default C++ Compiler: ', env['CXX']
else:
    print 'Using user supplied C++ Compiler: ', env['CXX']
    env.Replace(CXX = GetOption('cxx'))
if GetOption('stxxlroot') is not None:
   env.Append(CPPPATH = GetOption('stxxlroot')+'/include')
   env.Append(LIBPATH = GetOption('stxxlroot')+'/lib')
   print 'STXXLROOT = ', GetOption('stxxlroot')
if sys.platform == 'win32':
    #SCons really wants to use Microsoft compiler
    print "Compiling is not yet supported on Windows"
    Exit(-1)
else:  #Mac OS X
    if sys.platform == 'darwin':
        print "Compiling is experimental on Mac"
        env.Append(CPPPATH = ['/opt/local/include/', '/opt/local/include/libxml2'])
	env.Append(LIBPATH = ['/opt/local/lib'])
    else:
        env.Append(CPPPATH = ['/usr/include', '/usr/include/include', '/usr/include/libxml2/'])
if GetOption('buildconfiguration') == 'debug':
	env.Append(CCFLAGS = ' -g3')
else:
	env.Append(CCFLAGS = ' -O3')
#print "Compiling with: ", env['CXX']
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
if not conf.CheckLibWithHeader('bz2', 'bzlib.h', 'CXX'):
	print "bz2 library not found. Exiting"
	Exit(-1)
if not conf.CheckLib('boost_thread'):
	if not conf.CheckLib('boost_thread-mt'):
		print "boost thread library not found. Exiting"
		Exit(-1)
	else:
		env.Append(CCFLAGS = ' -lboost_thread-mt')
		env.Append(LINKFLAGS = ' -lboost_thread-mt')
else:
	env.Append(CCFLAGS = ' -lboost_thread-mt')
	env.Append(LINKFLAGS = ' -lboost_thread-mt')
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
if not conf.CheckLib('stxxl'):
	print "stxxl library not found. Exiting"
	Exit(-1)

env.Append(CCFLAGS = ' -fopenmp')
env.Append(LINKFLAGS = ' -fopenmp')
env.Program("extractNetwork.cpp")
env.Program("extractLargeNetwork.cpp")	
env.Program("createHierarchy.cpp")
env.Append(CCFLAGS = ' -lboost_regex -lboost_iostreams -lboost_system -lbz2')
env.Append(LINKFLAGS = ' -lboost_regex -lboost_iostreams -lboost_system -lbz2')
env.Program("routed.cpp")
env = conf.Finish()

