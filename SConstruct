#Sconstruct

import os
import os.path
import string
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

# Adding various options to the SConstruct
AddOption('--cxx', dest='cxx', type='string', nargs=1, action='store', metavar='STRING', help='C++ Compiler')
AddOption('--stxxlroot', dest='stxxlroot', type='string', nargs=1, action='store', metavar='STRING', help='root directory of STXXL')
AddOption('--verbosity', dest='verbosity', type='string', nargs=1, action='store', metavar='STRING', help='make Scons talking')
AddOption('--buildconfiguration', dest='buildconfiguration', type='string', nargs=1, action='store', metavar='STRING', help='debug or release')
AddOption('--all-flags', dest='allflags', type='string', nargs=0, action='store', metavar='STRING', help='turn off -march optimization in release mode')
AddOption('--with-tools', dest='withtools', type='string', nargs=0, action='store', metavar='STRING', help='build tools for data analysis')
AddOption('--no-march', dest='nomarch', type='string', nargs=0, action='store', metavar='STRING', help='turn off native optimizations')

env = Environment( ENV = {'PATH' : os.environ['PATH']} ,COMPILER = GetOption('cxx'))
env["CC"] = os.getenv("CC") or env["CC"]
env["CXX"] = os.getenv("CXX") or env["CXX"]
env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))
try:
	env['ENV']['TERM'] = os.environ['TERM']
except KeyError:
	env['ENV']['TERM'] = 'none'
	
conf = Configure(env, custom_tests = { 'CheckBoost' : CheckBoost, 'CheckProtobuf' : CheckProtobuf })

if GetOption('cxx') is None:
    #default Compiler
    if sys.platform == 'darwin':	#Mac OS X
        env['CXX'] = 'clang++'
    print 'Using default C++ Compiler: ', env['CXX'].strip()
else:
    env.Replace(CXX = GetOption('cxx'))
    print 'Using user supplied C++ Compiler: ', env['CXX']

if GetOption('allflags') is not None:
    env.Append(CXXFLAGS = ["-Wextra", "-Wall", "-Wnon-virtual-dtor", "-Wundef", "-Wno-long-long", "-Woverloaded-virtual", "-Wfloat-equal", "-Wredundant-decls"])

if "clang" in env["CXX"]:
    print "Warning building with clang removes OpenMP parallelization"
    if GetOption('allflags') is not None:
        env.Append(CXXFLAGS = ["-W#warnings", "-Wc++0x-compat", "-Waddress-of-temporary", "-Wambiguous-member-template", "-Warray-bounds", "-Watomic-properties", "-Wbind-to-temporary-copy", "-Wbuiltin-macro-redefined", "-Wc++-compat", "-Wc++0x-extensions", "-Wcomments", "-Wconditional-uninitialized", "-Wconstant-logical-operand", "-Wdeclaration-after-statement", "-Wdeprecated", "-Wdeprecated-implementations", "-Wdeprecated-writable-strings", "-Wduplicate-method-arg", "-Wempty-body", "-Wendif-labels", "-Wenum-compare", "-Wformat=2", "-Wfour-char-constants", "-Wgnu", "-Wincomplete-implementation", "-Winvalid-noreturn", "-Winvalid-offsetof", "-Winvalid-token-paste", "-Wlocal-type-template-args", "-Wmethod-signatures", "-Wmicrosoft", "-Wmissing-declarations", "-Wnon-pod-varargs", "-Wnonfragile-abi2", "-Wnull-dereference", "-Wout-of-line-declaration", "-Woverlength-strings", "-Wpacked", "-Wpointer-arith", "-Wpointer-sign", "-Wprotocol", "-Wreadonly-setter-attrs", "-Wselector", "-Wshift-overflow", "-Wshift-sign-overflow", "-Wstrict-selector-match", "-Wsuper-class-method-mismatch", "-Wtautological-compare", "-Wtypedef-redefinition", "-Wundeclared-selector", "-Wunnamed-type-template-args", "-Wunused-exception-parameter", "-Wunused-member-function", "-Wused-but-marked-unused", "-Wvariadic-macros"])
else:
	env.Append(CCFLAGS = ['-minline-all-stringops', '-fopenmp'])
	env.Append(LINKFLAGS = '-fopenmp')

if GetOption('buildconfiguration') == 'debug':
	env.Append(CCFLAGS = ['-Wall', '-g3', '-rdynamic'])
else:
	env.Append(CCFLAGS = ['-O3', '-DNDEBUG'])

if sys.platform == 'darwin':	#Mac OS X
	#os x default installations
	env.Append(CPPPATH = ['/usr/include/libxml2'] )
	env.Append(CPPPATH = ['/usr/X11/include'])		#comes with os x
#	env.Append(LIBPATH = ['/usr/X11/lib'])			#needed for libpng
	
	#assume stxxl and boost are installed via homebrew. call brew binary to get folder locations
	import subprocess
	stxxl_prefix = subprocess.check_output(["brew", "--prefix", "libstxxl"]).strip()
	env.Append(CPPPATH = [stxxl_prefix+"/include"] )
	env.Append(LIBPATH = [stxxl_prefix+"/lib"] )
	boost_prefix = subprocess.check_output(["brew", "--prefix", "boost"]).strip()
	env.Append(CPPPATH = [boost_prefix+"/include"] )
	env.Append(LIBPATH = [boost_prefix+"/lib"] )
	if not conf.CheckLibWithHeader('lua', 'lua.h', 'C'):
		print "lua library not found. Exiting"
		Exit(-1)
	
	if not conf.CheckLibWithHeader('luabind', 'luabind/luabind.hpp', 'CXX'):
		print "luabind library not found. Exiting"
		Exit(-1)

elif sys.platform.startswith("freebsd"):
	env.ParseConfig('pkg-config --cflags --libs protobuf')
	env.Append(CPPPATH = ['/usr/local/include', '/usr/local/include/libxml2'])
	env.Append(LIBPATH = ['/usr/local/lib'])
	if GetOption('stxxlroot') is not None:
	   env.Append(CPPPATH = GetOption('stxxlroot')+'/include')
	   env.Append(LIBPATH = GetOption('stxxlroot')+'/lib')
	   print 'STXXLROOT = ', GetOption('stxxlroot')
elif sys.platform == 'win32':
	#SCons really wants to use Microsoft compiler
	print "Compiling is not yet supported on Windows"
	Exit(-1)
else:
	print "Default platform"
	if GetOption('stxxlroot') is not None:
	   env.Append(CPPPATH = GetOption('stxxlroot')+'/include')
	   env.Append(LIBPATH = GetOption('stxxlroot')+'/lib')
	   print 'STXXLROOT = ', GetOption('stxxlroot')
	env.Append(CPPPATH = ['/usr/include', '/usr/include/include', '/usr/include/libxml2/'])
	if not conf.CheckLibWithHeader('pthread', 'pthread.h', 'CXX'):
		print "pthread not found. Exiting"
		Exit(-1)
	
	if not conf.CheckLibWithHeader('luajit-5.1', 'luajit-2.0/lua.h', 'CXX'):
		print "luajit library not found. Checking for interpreter"
		env.ParseConfig('pkg-config --cflags --libs lua5.1')
	env.ParseConfig('pkg-config --cflags --libs luabind')

#Check if architecture optimizations shall be turned off
if GetOption('buildconfiguration') != 'debug' and sys.platform != 'darwin' and GetOption('nomarch') is None:
	env.Append(CCFLAGS = ['-march=native'])

if not conf.CheckHeader('omp.h'):
	print "Compiler does not support OpenMP. Exiting"
	Exit(-1)
if not conf.CheckLibWithHeader('bz2', 'bzlib.h', 'CXX'):
	print "bz2 library not found. Exiting"
	Exit(-1)
if GetOption('withtools') is not None:
	if not conf.CheckLibWithHeader('gdal', 'gdal/gdal.h', 'CXX'):
		print "gdal library not found. Exiting"
		Exit(-1)
if not conf.CheckLibWithHeader('osmpbf', 'osmpbf/osmpbf.h', 'CXX'):
	print "osmpbf library not found. Exiting"
	print "Either install libosmpbf-dev (Ubuntu) or use https://github.com/scrosby/OSM-binary"
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
#Check BOOST installation
if not (conf.CheckBoost('1.44')):
	print 'Boost version >= 1.44 needed'
	Exit(-1);
if not conf.CheckLib('boost_system', language="C++"):
	if not conf.CheckLib('boost_system-mt', language="C++"):
		print "boost_system library not found. Exiting"
		Exit(-1)
	else:
		print "using boost -mt"
		env.Append(CCFLAGS = ' -lboost_system-mt')
		env.Append(LINKFLAGS = ' -lboost_system-mt')
if not conf.CheckLibWithHeader('boost_thread', 'boost/thread.hpp', 'CXX'):
	if not conf.CheckLibWithHeader('boost_thread-mt', 'boost/thread.hpp', 'CXX'):
		print "boost thread library not found. Exiting"
		Exit(-1)
	else:
		print "using boost -mt"
		env.Append(CCFLAGS = ' -lboost_thread-mt')
		env.Append(LINKFLAGS = ' -lboost_thread-mt')
if not conf.CheckLibWithHeader('boost_regex', 'boost/regex.hpp', 'CXX'):
	if not conf.CheckLibWithHeader('boost_regex-mt', 'boost/regex.hpp', 'CXX'):
		print "boost/regex.hpp not found. Exiting"
		Exit(-1)
	else:
		print "using boost_regex -mt"
		env.Append(CCFLAGS = ' -lboost_regex-mt')
		env.Append(LINKFLAGS = ' -lboost_regex-mt')
if not conf.CheckLib('boost_filesystem', language="C++"):
	if not conf.CheckLib('boost_filesystem-mt', language="C++"):
		print "boost_filesystem library not found. Exiting"
		Exit(-1)
	else:
		print "using boost -mt"
		env.Append(CCFLAGS = ' -lboost_filesystem-mt')
		env.Append(LINKFLAGS = ' -lboost_filesystem-mt')
if not conf.CheckCXXHeader('boost/archive/iterators/base64_from_binary.hpp'):
	print "boost/archive/iterators/base64_from_binary.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/archive/iterators/binary_from_base64.hpp'):
	print "boost/archive/iterators/binary_from_base64.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/archive/iterators/transform_width.hpp'):
	print "boost/archive/iterators/transform_width.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/bind.hpp'):
	print "boost/bind.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/circular_buffer.hpp'):
	print "boost/circular_buffer.hpp not found. Exiting"
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
if not conf.CheckCXXHeader('boost/thread/mutex.hpp'):
	print "boost/shared_ptr.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/thread/thread.hpp'):
	print "boost/thread/thread.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/thread/condition.hpp'):
	print "boost/thread/condition.hpp not found. Exiting"
	Exit(-1)
if not conf.CheckCXXHeader('boost/thread/thread.hpp'):
	print "boost/thread/thread.hpp not found. Exiting"
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

#checks for intels thread building blocks library
#if not conf.CheckLibWithHeader('tbb', 'tbb/tbb.h', 'CXX'):
#	print "Intel TBB library not found. Exiting"
#	Exit(-1)
#if not conf.CheckCXXHeader('tbb/task_scheduler_init.h'):
#	print "tbb/task_scheduler_init.h not found. Exiting"
#	Exit(-1)

env.Program(target = 'osrm-extract', source = ["extractor.cpp", Glob('Util/*.cpp'), Glob('Extractor/*.cpp')])
env.Program(target = 'osrm-prepare', source = ["createHierarchy.cpp", Glob('Contractor/*.cpp'), Glob('Util/SRTMLookup/*.cpp'), Glob('Algorithms/*.cpp')])
env.Program(target = 'osrm-routed', source = ["routed.cpp", 'Descriptors/DescriptionFactory.cpp', Glob('ThirdParty/*.cc'), Glob('Server/DataStructures/*.cpp')], CCFLAGS = env['CCFLAGS'] + ['-DROUTED'])
if GetOption('withtools') is not None:
	env.Program(target = 'Tools/osrm-component', source = ["Tools/componentAnalysis.cpp"])
env = conf.Finish()

