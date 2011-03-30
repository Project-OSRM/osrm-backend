#Sconstruct

import os
import os.path
import sys
from subprocess import call

def CheckBoost(context, version):
    # Boost versions are in format major.minor.subminor
    v_arr = version.split(".")
    version_n = 0
    if len(v_arr) > 0:
        version_n += int(v_arr[0])*100000
    if len(v_arr) > 1:
        version_n += int(v_arr[1])*100
    if len(v_arr) > 2:
        version_n += int(v_arr[2])
        
    context.Message('Checking for Boost version >= %s... ' % (version))
    ret = context.TryRun("""
    #include <boost/version.hpp>

    int main() 
    {
        return BOOST_VERSION >= %d ? 0 : 1;
    }
    """ % version_n, '.cpp')[0]
    context.Result(ret)
    return ret

def CheckProtobuf(context, version):
    # Protobuf versions are in format major.minor.subminor
    v_arr = version.split(".")
    version_n = 0
    if len(v_arr) > 0:
        version_n += int(v_arr[0])*1000000
    if len(v_arr) > 1:
        version_n += int(v_arr[1])*1000
    if len(v_arr) > 2:
        version_n += int(v_arr[2])
        
    context.Message('Checking for Protobuffer version >= %s... ' % (version))
    ret = context.TryRun("""
    #include <google/protobuf/stubs/common.h>
	int main() {
	return (GOOGLE_PROTOBUF_VERSION >= %d) ? 0 : 1;
    }
    """ % version_n, '.cpp')[0]
    context.Result(ret)
    return ret

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
	env.Append(CCFLAGS = ' -Wall -g3')
else:
	env.Append(CCFLAGS = ' -O3 -DNDEBUG -march=native')
#print "Compiling with: ", env['CXX']
conf = Configure(env, custom_tests = { 'CheckBoost' : CheckBoost, 'CheckProtobuf' : CheckProtobuf })
if not conf.CheckHeader('omp.h'):
	print "Compiler does not support OpenMP. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('xml2', 'libxml/xmlreader.h', 'CXX'):
	print "libxml2 library or header not found. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('z', 'zlib.h', 'CXX'):
	print "zlib library or header not found. Exiting"
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
if not conf.CheckLibWithHeader('boost_thread', 'boost/thread.hpp', 'CXX'):
	if not conf.CheckLibWithHeader('boost_thread-mt', 'boost/thread.hpp', 'CXX'):
		print "boost thread library not found. Exiting"
		Exit(-1)
	else:
		print "using boost -mt"
		env.Append(CCFLAGS = ' -lboost_thread-mt')
		env.Append(LINKFLAGS = ' -lboost_thread-mt')
if not conf.CheckCXXHeader('boost/thread.hpp'):
	print "boost thread header not found. Exiting"
	Exit(-1)
if not conf.CheckLib('boost_system', language="C++"):
	print "boost_system library not found. Exiting"
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
if not conf.CheckLibWithHeader('stxxl', 'stxxl.h', 'CXX'):
	print "stxxl library not found. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('protobuf', 'google/protobuf/descriptor.h', 'CXX'):
	print "Google Protobuffer library not found. Exiting"
	Exit(-1)
#if os.sysconf('SC_NPROCESSORS_ONLN') > 1:
#	env.Append(CCFLAGS = ' -D_GLIBCXX_PARALLEL');
if not (conf.CheckBoost('1.37')):
	print 'Boost version >= 1.37 needed'
	Exit(-1);
#check for protobuf 2.3.0, else rebuild proto files
if not (conf.CheckProtobuf('2.3.0')):
	if not (env.Detect('protoc')):
		print 'protobuffer compiler not found'
		Exit(-1);

protobld = Builder(action = 'protoc -I=DataStructures/pbf-proto --cpp_out=DataStructures/pbf-proto $SOURCE')
env.Append(BUILDERS = {'Protobuf' : protobld})
env.Protobuf('DataStructures/pbf-proto/fileformat.proto')
env.Protobuf('DataStructures/pbf-proto/osmformat.proto')
env.Append(CCFLAGS = ' -fopenmp')
env.Append(LINKFLAGS = ' -fopenmp')


env.StaticObject("DataStructures/pbf-proto/fileformat.pb.cc")
env.StaticObject("DataStructures/pbf-proto/osmformat.pb.cc")
env.Program("extractor.cpp")
env.Program("createHierarchy.cpp")
if os.path.exists("many-to-many.cpp"):
	env.Program("many-to-many.cpp")
env.Append(CCFLAGS = ' -lboost_regex -lboost_iostreams -lbz2 -lz -lprotobuf')
env.Append(LINKFLAGS = '-lboost_system DataStructures/pbf-proto/fileformat.pb.o DataStructures/pbf-proto/osmformat.pb.o')
env.Program("routed.cpp")
env = conf.Finish()

