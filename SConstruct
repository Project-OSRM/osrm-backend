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
env = Environment(ENV = {'PATH' : os.environ['PATH']} ,COMPILER = GetOption('cxx'))
if sys.platform.startswith("freebsd"):
	env.ParseConfig('pkg-config --cflags --libs protobuf')
if GetOption('cxx') is None:
    #default Compiler
    print 'Using default C++ Compiler: ', env['CXX']
else:
    env.Replace(CXX = GetOption('cxx'))
    print 'Using user supplied C++ Compiler: ', env['CXX']
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
    elif sys.platform.startswith('freebsd'):
        env.Append(CPPPATH = ['/usr/local/include', '/usr/local/include/libxml2'])
        env.Append(LIBPATH = ['/usr/local/lib'])
    else:
        env.Append(CPPPATH = ['/usr/include', '/usr/include/include', '/usr/include/libxml2/'])
if GetOption('buildconfiguration') == 'debug':
	env.Append(CCFLAGS = ['-Wall', '-g3', '-rdynamic'])
else:
	env.Append(CCFLAGS = ['-O3', '-DNDEBUG', '-march=native'])
#print "Compiling with: ", env['CXX']
conf = Configure(env, custom_tests = { 'CheckBoost' : CheckBoost, 'CheckProtobuf' : CheckProtobuf })
if not conf.CheckHeader('omp.h'):
	print "Compiler does not support OpenMP. Exiting"
	if sys.platform == 'darwin':
		print "Continuing because we are on Mac. This might be fatal."
	else:
		Exit(-1)
if not conf.CheckLibWithHeader('bz2', 'bzlib.h', 'CXX'):
	print "bz2 library not found. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('png', 'png.h', 'CXX'):
	print "png library or header not found. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('pthread', 'pthread.h', 'CXX'):
        print "pthread not found. Exiting"
        Exit(-1)
if not conf.CheckLibWithHeader('protobuf', 'google/protobuf/descriptor.h', 'CXX'):
	print "Google Protobuffer library not found. Exiting"
	Exit(-1)
#check for protobuf 2.3.0
if not (conf.CheckProtobuf('2.3.0')):
	print 'libprotobuf version >= 2.3.0 needed'
	Exit(-1);
if not (env.Detect('protoc')):
	print 'protobuffer compiler not found'
	Exit(-1);
if not conf.CheckLibWithHeader('stxxl', 'stxxl.h', 'CXX'):
	print "stxxl library not found. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('xml2', 'libxml/xmlreader.h', 'CXX'):
	print "libxml2 library or header not found. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('z', 'zlib.h', 'CXX'):
	print "zlib library or header not found. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('zip', 'zip.h', 'CXX'):
	print "Zip library not found. Exiting"
	Exit(-1)
#Check BOOST installation
if not (conf.CheckBoost('1.41')):
	print 'Boost version >= 1.41 needed'
	Exit(-1);
if not conf.CheckLib('boost_system', language="C++"):
	print "boost_system library not found. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('boost_thread', 'boost/thread.hpp', 'CXX'):
	if not conf.CheckLibWithHeader('boost_thread-mt', 'boost/thread.hpp', 'CXX'):
		print "boost thread library not found. Exiting"
		Exit(-1)
	else:
		print "using boost -mt"
		env.Append(CCFLAGS = ' -lboost_thread-mt')
		env.Append(LINKFLAGS = ' -lboost_thread-mt')
if not conf.CheckLibWithHeader('boost_regex', 'boost/regex.hpp', 'CXX'):
	print "boost/regex.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/array.hpp'):
	print "boost/thread.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/asio.hpp'):
	print "boost/thread.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/bind.hpp'):
	print "boost/bind.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/enable_shared_from_this.hpp'):
	print "boost/bind.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/foreach.hpp'):
	print "boost/foreach.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/lexical_cast.hpp'):
	print "boost/foreach.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/logic/tribool.hpp'):
	print "boost/foreach.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/math/tr1.hpp'):
	print "boost/foreach.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/noncopyable.hpp'):
	print "boost/noncopyable.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/property_tree/ptree.hpp'):
	print "boost/property_tree/ptree.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/property_tree/ini_parser.hpp'):
	print "boost/property_tree/ini_parser.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/shared_ptr.hpp'):
	print "boost/shared_ptr.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/signals2/mutex.hpp'):
	print "boost/shared_ptr.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/thread.hpp'):
	print "boost thread header not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/tuple/tuple.hpp'):
	print "boost thread header not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/unordered_map.hpp'):
	print "boost thread header not found. Exiting"
	Exit(-1)

protobld = Builder(action = 'protoc -I=DataStructures/pbf-proto --cpp_out=DataStructures/pbf-proto $SOURCE')
env.Append(BUILDERS = {'Protobuf' : protobld})
env.Protobuf('DataStructures/pbf-proto/fileformat.proto')
env.Protobuf('DataStructures/pbf-proto/osmformat.proto')
env.Append(CCFLAGS = ['-fopenmp'])
env.Append(LINKFLAGS = ['-fopenmp'])

env.Program(target = 'osrm-extract', source = ["extractor.cpp", Glob('DataStructures/pbf-proto/*.pb.cc'), Glob('Util/*.cpp')])
env.Program(target = 'osrm-prepare', source = ["createHierarchy.cpp", 'Contractor/EdgeBasedGraphFactory.cpp', Glob('Util/SRTMLookup/*.cpp'), Glob('Algorithms/*.cpp')])
env.Append(CCFLAGS = ['-lboost_regex', '-lboost_iostreams', '-lbz2', '-lz', '-lprotobuf'])
env.Append(LINKFLAGS = ['-lboost_system'])
env.Program(target = 'osrm-routed', source = ["routed.cpp", 'Descriptors/DescriptionFactory.cpp', Glob('ThirdParty/*.cc')], CCFLAGS = env['CCFLAGS'] + ['-DROUTED'])
env = conf.Finish()

