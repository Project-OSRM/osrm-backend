cmake_minimum_required(VERSION 2.8.11)
# we depend on 2.8.11 introducing target_include_directories

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR AND NOT MSVC_IDE)
  message(FATAL_ERROR "In-source builds are not allowed.
Please create a directory and run cmake from there, passing the path to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'. Please delete them.")
endif()

# detect if this is included as subproject and if so expose
# some variables to its parent scope
get_directory_property(BUILD_AS_SUBPROJECT PARENT_DIRECTORY)
if(BUILD_AS_SUBPROJECT)
  message(STATUS "Building libosrm as subproject.")
endif()

option(ENABLE_MASON "Use mason for dependencies" OFF)
option(ENABLE_CCACHE "Speed up incremental rebuilds via ccache" ON)
option(BUILD_TOOLS "Build OSRM tools" OFF)
option(BUILD_PACKAGE "Build OSRM package" OFF)
option(ENABLE_ASSERTIONS "Use assertions in release mode" OFF)
option(ENABLE_COVERAGE "Build with coverage instrumentalisation" OFF)
option(ENABLE_SANITIZER "Use memory sanitizer for Debug build" OFF)
option(ENABLE_LTO "Use LTO if available" OFF)
option(ENABLE_FUZZING "Fuzz testing using LLVM's libFuzzer" OFF)
option(ENABLE_GOLD_LINKER "Use GNU gold linker if available" ON)
option(ENABLE_NODE_BINDINGS "Build NodeJs bindings" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(ENABLE_MASON)
  # versions in use
  set(MASON_BOOST_VERSION "1.63.0")
  set(MASON_STXXL_VERSION "1.4.1-1")
  set(MASON_EXPAT_VERSION "2.2.0")
  set(MASON_LUA_VERSION "5.2.4")
  set(MASON_BZIP2_VERSION "1.0.6")
  set(MASON_TBB_VERSION "2017_20161128")
  set(MASON_LIBSHP_VERSION "1.3.0")

  message(STATUS "Enabling mason")

  find_program(CURL_FOUND curl)
  if(NOT CURL_FOUND)
    message(FATAL_ERROR "curl command required with -DENABLE_MASON")
  endif()

  include(mason)
endif()

# be compatible with version handling before cmake 3.x
if (POLICY CMP0048)
  cmake_policy(SET CMP0048 OLD)
endif()
project(OSRM C CXX)
set(OSRM_VERSION_MAJOR 5)
set(OSRM_VERSION_MINOR 8)
set(OSRM_VERSION_PATCH 0)
set(OSRM_VERSION "${OSRM_VERSION_MAJOR}.${OSRM_VERSION_MINOR}.${OSRM_VERSION_PATCH}")

add_definitions(-DOSRM_PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# these two functions build up custom variables:
#   DEPENDENCIES_INCLUDE_DIRS and OSRM_DEFINES
# These variables we want to pass to
# include_directories and add_definitions for both
# this build and for sharing externally via pkg-config

function(add_dependency_includes)
  if(${ARGC} GREATER 0)
    list(APPEND DEPENDENCIES_INCLUDE_DIRS "${ARGV}")
    set(DEPENDENCIES_INCLUDE_DIRS "${DEPENDENCIES_INCLUDE_DIRS}" PARENT_SCOPE)
  endif()
endfunction(add_dependency_includes)

function(add_dependency_defines defines)
  list(APPEND OSRM_DEFINES "${defines}")
  set(OSRM_DEFINES "${OSRM_DEFINES}" PARENT_SCOPE)
endfunction(add_dependency_defines)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CheckCXXCompilerFlag)
include(FindPackageHandleStandardArgs)
include(GNUInstallDirs)

set(bitness 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(bitness 64)
  message(STATUS "Building on a 64 bit system")
else()
  message(STATUS "Building on a 32 bit system")
endif()

if(WIN32 AND MSVC_VERSION LESS 1900)
  message(FATAL_ERROR "Building with Microsoft compiler needs Latest Visual Studio 2015 (Community or better)")
endif()

# Strictly require GCC>=4.9 and Clang>=3.4 - GCC 4.8 is already too old for C++14.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    message(FATAL_ERROR "GCC>=4.9 required. In case you are on Ubuntu upgrade via ppa:ubuntu-toolchain-r/test")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.4)
    message(FATAL_ERROR "Clang>=3.4 required. In case you are on Ubuntu upgrade via http://apt.llvm.org")
  endif()
endif()

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}/include/)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sol2/)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/variant/include)

set(BOOST_COMPONENTS date_time chrono filesystem iostreams program_options regex system thread unit_test_framework)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/util/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/util/version.hpp
)
file(GLOB UtilGlob src/util/*.cpp src/util/*/*.cpp)
file(GLOB ExtractorGlob src/extractor/*.cpp src/extractor/*/*.cpp)
file(GLOB PartitionerGlob src/partition/*.cpp)
file(GLOB CustomizerGlob src/customize/*.cpp)
file(GLOB ContractorGlob src/contractor/*.cpp)
file(GLOB UpdaterGlob src/updater/*.cpp)
file(GLOB StorageGlob src/storage/*.cpp)
file(GLOB ServerGlob src/server/*.cpp src/server/**/*.cpp)
file(GLOB EngineGlob src/engine/*.cpp src/engine/**/*.cpp)

add_library(UTIL OBJECT ${UtilGlob})
add_library(EXTRACTOR OBJECT ${ExtractorGlob})
add_library(PARTITIONER OBJECT ${PartitionerGlob})
add_library(CUSTOMIZER OBJECT ${CustomizerGlob})
add_library(CONTRACTOR OBJECT ${ContractorGlob})
add_library(UPDATER OBJECT ${UpdaterGlob})
add_library(STORAGE OBJECT ${StorageGlob})
add_library(ENGINE OBJECT ${EngineGlob})
add_library(SERVER OBJECT ${ServerGlob})

set_target_properties(UTIL PROPERTIES LINKER_LANGUAGE CXX)

add_executable(osrm-extract src/tools/extract.cpp)
add_executable(osrm-partition src/tools/partition.cpp)
add_executable(osrm-customize src/tools/customize.cpp)
add_executable(osrm-contract src/tools/contract.cpp)
add_executable(osrm-routed src/tools/routed.cpp $<TARGET_OBJECTS:SERVER> $<TARGET_OBJECTS:UTIL>)
add_executable(osrm-datastore src/tools/store.cpp $<TARGET_OBJECTS:UTIL>)
add_library(osrm src/osrm/osrm.cpp $<TARGET_OBJECTS:ENGINE> $<TARGET_OBJECTS:UTIL> $<TARGET_OBJECTS:STORAGE>)
add_library(osrm_contract src/osrm/contractor.cpp $<TARGET_OBJECTS:CONTRACTOR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_extract src/osrm/extractor.cpp $<TARGET_OBJECTS:EXTRACTOR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_partition $<TARGET_OBJECTS:PARTITIONER> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_customize $<TARGET_OBJECTS:CUSTOMIZER> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_update $<TARGET_OBJECTS:UPDATER> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_store $<TARGET_OBJECTS:STORAGE> $<TARGET_OBJECTS:UTIL>)

if(ENABLE_GOLD_LINKER)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
    if("${LD_VERSION}" MATCHES "GNU gold")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold -Wl,--disable-new-dtags")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold -Wl,--disable-new-dtags")
        set(OSRM_LDFLAGS "${OSRM_LDFLAGS} -fuse-ld=gold -Wl,--disable-new-dtags")
        message(STATUS "Using GNU gold as linker.")

        # Issue 2785: check gold binutils version and don't use gc-sections for versions prior 2.25
        string(REGEX REPLACE ".*\\(GNU Binutils[^\\)0-9]+([0-9]+\\.[0-9]+)[^\\)]*\\).*" "\\1" GOLD_BINUTILS_VERSION "${LD_VERSION}")
        if ("${GOLD_BINUTILS_VERSION}" VERSION_LESS "2.26")
          message(STATUS "Disabling gc-sections on gold binutils < 2.26, see: https://sourceware.org/bugzilla/show_bug.cgi?id=17639")
          set(LD_AVOID_GC_SECTIONS TRUE)
        endif()
    else()
        message(WARNING "GNU gold linker isn't available.")
        set(ENABLE_GOLD_LINKER OFF)
    endif()
endif()

# Disable LTO when mason+gcc is detected before testing for / setting any flags.
# Mason builds libraries with Clang, mixing does not work in the context of lto.
if(ENABLE_MASON AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND ENABLE_LTO)
  set(ENABLE_LTO OFF)
  message(WARNING "Mason and GCC's LTO not work together. Disabling LTO.")
endif()

# Explicitly set the build type to Release if no other type is specified
# on the command line.  Without this, cmake defaults to an unoptimized,
# non-debug build, which almost nobody wants.
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type specified, defaulting to Release")
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Configuring OSRM in debug mode")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Configuring OSRM in release mode")
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  message(STATUS "Configuring OSRM in release mode with debug flags")
elseif(CMAKE_BUILD_TYPE MATCHES MinRelSize)
  message(STATUS "Configuring OSRM in release mode with minimized size")
else()
  message(STATUS "Unrecognized build type - will use cmake defaults")
endif()

# Additional logic for the different build types
if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  message(STATUS "Configuring debug mode flags")
  set(ENABLE_ASSERTIONS ON)
  if(NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline -fno-omit-frame-pointer")
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      if (CMAKE_BUILD_TYPE MATCHES Debug)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -ggdb")
      else()
        # Don't override the -O parameter for RelWithDebInfo, we want an optimized build
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
      endif()
    endif()
  endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES MinRelSize OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  message(STATUS "Configuring release mode optimizations")
  # Check if LTO is available
  check_cxx_compiler_flag("-Wl,-flto" LTO_AVAILABLE)

  if(ENABLE_LTO AND LTO_AVAILABLE)
    set(OLD_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    # GCC in addition allows parallelizing LTO
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      include(ProcessorCount)
      ProcessorCount(NPROC)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=${NPROC}")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    endif()
    set(CHECK_LTO_SRC "int main(){return 0;}")
    check_cxx_source_compiles("${CHECK_LTO_SRC}" LTO_WORKS)
    if(LTO_WORKS)
      message(STATUS "LTO working")
      set(OSRM_CXXFLAGS "${OSRM_CXXFLAGS} -flto")
      set(OSRM_LDFLAGS "${OSRM_LDFLAGS} -flto")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -flto")
    else()
      message(STATUS "LTO broken")
      set(CMAKE_CXX_FLAGS "${OLD_CXX_FLAGS}")
      set(ENABLE_LTO Off)
    endif()

    # Since gcc 4.9 the LTO format is non-standart ('slim'), so we need to use the build-in tools
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND
        NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.9.0" AND NOT MINGW)
      find_program(GCC_AR gcc-ar)
      find_program(GCC_RANLIB gcc-ranlib)
      if ("${GCC_AR}" STREQUAL "GCC_AR-NOTFOUND" OR "${GCC_RANLIB}" STREQUAL "GCC_RANLIB-NOTFOUND")
        message(WARNING "GCC specific binutils not found. In case of linker issues export env vars: AR=gcc-ar, NM=gcc-nm, RANLIB=gcc-ranlib")
      else()
        message(STATUS "Using GCC specific binutils for LTO:")
        message(STATUS " ${GCC_AR}")
        message(STATUS " ${GCC_RANLIB}")
        set(CMAKE_AR ${GCC_AR})
        set(CMAKE_RANLIB ${GCC_RANLIB})
      endif()
    endif()

    # Same for clang LTO requires their own toolchain
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      find_program(LLVM_AR llvm-ar)
      find_program(LLVM_RANLIB llvm-ranlib)
      if ("${LLVM_AR}" STREQUAL "LLVM_AR-NOTFOUND" OR "${LLVM_RANLIB}" STREQUAL "LLVM_RANLIB-NOTFOUND")
        message(WARNING "LLVM specific binutils not found.")
      else()
        message(STATUS "Using LLVM specific binutils for LTO:")
        message(STATUS " ${LLVM_AR}")
        message(STATUS " ${LLVM_RANLIB}")
        set(CMAKE_AR ${LLVM_AR})
        set(CMAKE_RANLIB ${LLVM_RANLIB})
      endif()
    endif()

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.9.0")
      message(STATUS "Disabling LTO on GCC < 4.9.0 since it is broken, see: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57038")
      set(CMAKE_CXX_FLAGS "${OLD_CXX_FLAGS}")
      set(ENABLE_LTO Off)
    endif()
  endif()
endif()

if(UNIX AND NOT APPLE AND ENABLE_MASON AND (LTO_WORKS OR ENABLE_GOLD_LINKER))
  message(WARNING "ENABLE_MASON and ENABLE_LTO/ENABLE_GOLD_LINKER may not work on all linux systems currently")
  message(WARNING "For more details see: https://github.com/Project-OSRM/osrm-backend/issues/3202")
endif()

set(MAYBE_COVERAGE_LIBRARIES "")
if (ENABLE_COVERAGE)
  if (NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    message(ERROR "ENABLE_COVERAGE=ON only make sense with a Debug build")
  endif()
  message(INFO "Enabling coverage")
  set(MAYBE_COVERAGE_LIBRARIES "-lgcov")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs")
endif()
if (ENABLE_SANITIZER)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(OSRM_CXXFLAGS "${OSRM_CXXFLAGS} -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
endif()

# Configuring compilers
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wuninitialized -Wunreachable-code -Wstrict-overflow=2 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIC -fcolor-diagnostics -ftemplate-depth=1024 -Wno-unused-command-line-argument")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  set(COLOR_FLAG "-fdiagnostics-color=auto")
  check_cxx_compiler_flag("-fdiagnostics-color=auto" HAS_COLOR_FLAG)
  if(NOT HAS_COLOR_FLAG)
    set(COLOR_FLAG "")
  endif()
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wuninitialized -Wunreachable-code -Wstrict-overflow=1 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 ${COLOR_FLAG} -fPIC -ftemplate-depth=1024")
  if(WIN32) # using mingw
    add_dependency_defines(-DWIN32)
    set(OPTIONAL_SOCKET_LIBS ws2_32 wsock32)
  endif()

  # -fpermissive is required for parallel_do Intel TBB internal issue with GCC < 5
  # https://github.com/Project-OSRM/osrm-backend/pull/3603#issuecomment-277688589
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
    message(STATUS "Adding -fpermissive for GCC version < 5 bug (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51048). See #3603.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
  endif()

elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
  # using Intel C++
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-intel -wd10237 -Wall -ipo -fPIC")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
  # using Visual Studio C++
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} zlib)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj") # avoid compiler error C1128 from scripting_environment_lua.cpp
  add_dependency_defines(-DBOOST_LIB_DIAGNOSTIC)
  add_dependency_defines(-D_CRT_SECURE_NO_WARNINGS)
  add_dependency_defines(-DNOMINMAX) # avoid min and max macros that can break compilation
  add_dependency_defines(-D_USE_MATH_DEFINES) #needed for M_PI with cmath.h
  add_dependency_defines(-D_WIN32_WINNT=0x0501)
  add_dependency_defines(-DXML_STATIC)
  find_library(ws2_32_LIBRARY_PATH ws2_32)
  target_link_libraries(osrm-extract wsock32 ws2_32)
endif()

# Configuring linker
execute_process(COMMAND ${CMAKE_CXX_COMPILER} "-Wl,--version" ERROR_QUIET OUTPUT_VARIABLE LINKER_VERSION)
# For ld.gold and ld.bfs (the GNU linkers) we optimize hard
if("${LINKER_VERSION}" MATCHES "GNU gold" OR "${LINKER_VERSION}" MATCHES "GNU ld")
  message(STATUS "Setting linker optimizations")
  if(NOT (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC" OR "${LD_AVOID_GC_SECTIONS}"))
    # Tell compiler to put every function in separate section, linker can then match sections and functions
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    # Tell linker to do dead code and data eminination during link time discarding sections
    set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,--gc-sections")
  endif()
  # Default linker optimization flags
  set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,-O1 -Wl,--hash-style=gnu -Wl,--sort-common")

else()
  message(STATUS "Using unknown linker, not setting linker optimizations")
endif ()
set(OSRM_LDFLAGS "${OSRM_LDFLAGS} ${LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

# Activate C++1y
if(NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
  set(OSRM_CXXFLAGS "${OSRM_CXXFLAGS} -std=c++1y")
endif()

# Configuring other platform dependencies
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10")
  execute_process(COMMAND xcrun --sdk macosx --show-sdk-path OUTPUT_VARIABLE CMAKE_OSX_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(CMAKE_OSX_ARCHITECTURES "x86_64")
  message(STATUS "Set Architecture to x64 on OS X")
  exec_program(uname ARGS -v  OUTPUT_VARIABLE DARWIN_VERSION)
  string(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
  if(OSXLIBSTD)
    message(STATUS "linking against ${OSXLIBSTD}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=${OSXLIBSTD}")
  elseif(DARWIN_VERSION GREATER 12)
    message(STATUS "linking against libc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()
endif()

if(UNIX AND NOT APPLE)
  set(MAYBE_RT_LIBRARY -lrt)
endif()

# Disallow deprecated protozero APIs
add_definitions(-DPROTOZERO_STRICT_API)

find_package(Threads REQUIRED)

# if mason is enabled no find_package calls are made
# to ensure that we are only compiling and linking against
# fully portable mason packages
if(ENABLE_MASON)
  message(STATUS "Installing dependencies via mason")

  set(Boost_USE_STATIC_LIBS ON)
  mason_use(boost VERSION ${MASON_BOOST_VERSION} HEADER_ONLY)
  add_dependency_includes(${MASON_PACKAGE_boost_INCLUDE_DIRS})
  mason_use(boost_libfilesystem VERSION ${MASON_BOOST_VERSION})
  set(Boost_FILESYSTEM_LIBRARY ${MASON_PACKAGE_boost_libfilesystem_STATIC_LIBS})
  mason_use(boost_libiostreams VERSION ${MASON_BOOST_VERSION})
  set(Boost_IOSTREAMS_LIBRARY ${MASON_PACKAGE_boost_libiostreams_STATIC_LIBS})
  mason_use(boost_libprogram_options VERSION ${MASON_BOOST_VERSION})
  set(Boost_PROGRAM_OPTIONS_LIBRARY ${MASON_PACKAGE_boost_libprogram_options_STATIC_LIBS})
  mason_use(boost_libregex VERSION ${MASON_BOOST_VERSION})
  set(Boost_REGEX_LIBRARY ${MASON_PACKAGE_boost_libregex_STATIC_LIBS})
  mason_use(boost_libtest VERSION ${MASON_BOOST_VERSION})
  set(Boost_UNIT_TEST_FRAMEWORK_LIBRARY ${MASON_PACKAGE_boost_libtest_STATIC_LIBS})
  mason_use(boost_libdate_time VERSION ${MASON_BOOST_VERSION})
  set(Boost_DATE_TIME_LIBRARY ${MASON_PACKAGE_boost_libdate_time_STATIC_LIBS})
  mason_use(boost_libthread VERSION ${MASON_BOOST_VERSION})
  set(Boost_THREAD_LIBRARY ${MASON_PACKAGE_boost_libthread_STATIC_LIBS})
  mason_use(boost_libsystem VERSION ${MASON_BOOST_VERSION})
  set(Boost_SYSTEM_LIBRARY ${MASON_PACKAGE_boost_libsystem_STATIC_LIBS})

  mason_use(stxxl VERSION ${MASON_STXXL_VERSION})
  add_dependency_includes(${MASON_PACKAGE_stxxl_INCLUDE_DIRS})
  set(STXXL_LIBRARY ${MASON_PACKAGE_stxxl_STATIC_LIBS})

  mason_use(expat VERSION ${MASON_EXPAT_VERSION})
  add_dependency_includes(${MASON_PACKAGE_expat_INCLUDE_DIRS})
  set(EXPAT_LIBRARIES ${MASON_PACKAGE_expat_STATIC_LIBS})

  mason_use(lua VERSION ${MASON_LUA_VERSION})
  add_dependency_includes(${MASON_PACKAGE_lua_INCLUDE_DIRS})
  set(USED_LUA_LIBRARIES ${MASON_PACKAGE_lua_STATIC_LIBS})

  mason_use(bzip2 VERSION ${MASON_BZIP2_VERSION})
  add_dependency_includes(${MASON_PACKAGE_bzip2_INCLUDE_DIRS})
  set(BZIP2_LIBRARIES ${MASON_PACKAGE_bzip2_STATIC_LIBS})

  mason_use(tbb VERSION ${MASON_TBB_VERSION})
  add_dependency_includes(${MASON_PACKAGE_tbb_INCLUDE_DIRS})
  set(TBB_LIBRARIES ${MASON_PACKAGE_tbb_LDFLAGS})

  mason_use(libshp2 VERSION ${MASON_LIBSHP_VERSION})
  set(LIBSHAPEFILE_INCLUDE_DIR ${MASON_PACKAGE_libshp2_INCLUDE_DIRS})
  set(LIBSHAPEFILE_LIBRARY ${MASON_PACKAGE_libshp2_LDFLAGS})

  if(NOT MASON_PACKAGE_tbb_LIBRARY_DIRS)
    message(FATAL_ERROR "MASON_PACKAGE_tbb_LIBRARY_DIRS is empty, rpath will not work")
  endif()
  set(TBB_LINKER_RPATHS "")
  foreach(libpath ${MASON_PACKAGE_tbb_LIBRARY_DIRS})
    set(TBB_LINKER_RPATHS "${TBB_LINKER_RPATHS} -Wl,-rpath -Wl,${libpath}")
    file(GLOB TBBGlob ${libpath}/*.*)
    install(FILES ${TBBGlob} DESTINATION lib)
  endforeach()
  if(APPLE)
    set(LINKER_FLAGS "${TBB_LINKER_RPATHS} -Wl,-rpath -Wl,@executable_path")
  elseif(UNIX)
    set(LINKER_FLAGS "${TBB_LINKER_RPATHS} '-Wl,-rpath,$ORIGIN' -Wl,-z,origin")
  endif()

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

  # current mason packages target -D_GLIBCXX_USE_CXX11_ABI=0
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")

  # note: we avoid calling find_package(Osmium ...) here to ensure that the
  # expat and bzip2 are used from mason rather than the system
  include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium/include)

else()

  find_package(Boost 1.54 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
  add_dependency_includes(${Boost_INCLUDE_DIRS})
  if(WIN32 AND Boost_VERSION VERSION_LESS 106200)
    message(FATAL_ERROR "Building with MSVC needs Boost 1.62 with CXX11_CONSTEXPR support")
  endif()

  find_package(TBB REQUIRED)
  add_dependency_includes(${TBB_INCLUDE_DIR})
  if(WIN32 AND CMAKE_BUILD_TYPE MATCHES Debug)
    set(TBB_LIBRARIES ${TBB_DEBUG_LIBRARIES})
  endif()

  find_package(EXPAT REQUIRED)
  add_dependency_includes(${EXPAT_INCLUDE_DIRS})

  find_package(STXXL REQUIRED)
  add_dependency_includes(${STXXL_INCLUDE_DIR})

  find_package(BZip2 REQUIRED)
  add_dependency_includes(${BZIP2_INCLUDE_DIR})

  FIND_PACKAGE(Lua 5.2 EXACT)
  IF (LUA_FOUND)
      MESSAGE(STATUS "Using Lua ${LUA_VERSION_STRING}")
  ELSE()
    FIND_PACKAGE(Lua 5.1 EXACT)
    IF (LUA_FOUND)
      MESSAGE(STATUS "Using Lua ${LUA_VERSION_STRING}")
    ELSE()
      # Now fall back to a lua verison without exact
      # in case this cmake version also forces patch versions
      FIND_PACKAGE(Lua 5.2)
      IF (LUA_FOUND)
          MESSAGE(STATUS "Using Lua ${LUA_VERSION_STRING}")
      ELSE()
        FIND_PACKAGE(Lua 5.1)
        IF (LUA_FOUND)
          MESSAGE(STATUS "Using Lua ${LUA_VERSION_STRING}")
        ELSE()
          MESSAGE(FATAL_ERROR "Lua 5.1 or 5.2 was not found.")
        ENDIF()
      ENDIF()
    ENDIF()
  ENDIF()

  set(USED_LUA_LIBRARIES ${LUA_LIBRARIES})
  add_dependency_includes(${LUA_INCLUDE_DIR})

  # add a target to generate API documentation with Doxygen
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
  ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
  endif()

  # note libosmium depends on expat and bzip2
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium/cmake")
  if(NOT OSMIUM_INCLUDE_DIR)
    set(OSMIUM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium/include")
  endif()
  find_package(Osmium REQUIRED COMPONENTS io)
  include_directories(SYSTEM ${OSMIUM_INCLUDE_DIR})

endif()

# prefix compilation with ccache by default if available and on clang or gcc
if(ENABLE_CCACHE AND (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU"))
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    message(STATUS "Using ccache to speed up incremental builds")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    set(ENV{CCACHE_CPP2} "true")
  endif()
endif()

# even with mason builds we want to link to system zlib
# to ensure that osrm binaries play well with other binaries like nodejs
find_package(ZLIB REQUIRED)
add_dependency_includes(${ZLIB_INCLUDE_DIRS})

if(NOT WIN32 AND NOT Boost_USE_STATIC_LIBS)
  add_dependency_defines(-DBOOST_TEST_DYN_LINK)
endif()

if(NOT WIN32 AND NOT Boost_USE_STATIC_LIBS)
  add_dependency_defines(-DBOOST_TEST_DYN_LINK)
endif()

add_dependency_defines(-DBOOST_SPIRIT_USE_PHOENIX_V3)
add_dependency_defines(-DBOOST_RESULT_OF_USE_DECLTYPE)
add_dependency_defines(-DBOOST_FILESYSTEM_NO_DEPRECATED)

set(OpenMP_FIND_QUIETLY ON)
find_package(OpenMP)
if(OPENMP_FOUND)
  message(STATUS "OpenMP support found. Linking just in case for stxxl")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

add_definitions(${OSRM_DEFINES})
include_directories(SYSTEM ${DEPENDENCIES_INCLUDE_DIRS})

set(BOOST_BASE_LIBRARIES
   ${Boost_DATE_TIME_LIBRARY}
   ${Boost_CHRONO_LIBRARY}
   ${Boost_FILESYSTEM_LIBRARY}
   ${Boost_IOSTREAMS_LIBRARY}
   ${Boost_THREAD_LIBRARY}
   ${Boost_SYSTEM_LIBRARY})

set(BOOST_ENGINE_LIBRARIES
  ${Boost_ZLIB_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  ${BOOST_BASE_LIBRARIES})

# Binaries
target_link_libraries(osrm-datastore osrm_store ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(osrm-extract osrm_extract ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(osrm-partition osrm_partition ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(osrm-customize osrm_customize ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(osrm-contract osrm_contract ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(osrm-routed osrm ${Boost_PROGRAM_OPTIONS_LIBRARY} ${OPTIONAL_SOCKET_LIBS} ${ZLIB_LIBRARY})

set(EXTRACTOR_LIBRARIES
    ${BZIP2_LIBRARIES}
    ${Boost_REGEX_LIBRARY}
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXPAT_LIBRARIES}
    ${USED_LUA_LIBRARIES}
    ${OSMIUM_LIBRARIES}
    ${STXXL_LIBRARY}
    ${TBB_LIBRARIES}
    ${ZLIB_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(PARTITIONER_LIBRARIES
    ${BOOST_ENGINE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES}
    ${ZLIB_LIBRARY})
set(CUSTOMIZER_LIBRARIES
    ${BOOST_ENGINE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(UPDATER_LIBRARIES
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(CONTRACTOR_LIBRARIES
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${USED_LUA_LIBRARIES}
    ${STXXL_LIBRARY}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(ENGINE_LIBRARIES
    ${BOOST_ENGINE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES}
    ${ZLIB_LIBRARY})
set(STORAGE_LIBRARIES
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(UTIL_LIBRARIES
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${STXXL_LIBRARY}
    ${TBB_LIBRARIES}
    ${MAYBE_COVERAGE_LIBRARIES})
# Libraries
target_link_libraries(osrm ${ENGINE_LIBRARIES})
target_link_libraries(osrm_update ${UPDATER_LIBRARIES})
target_link_libraries(osrm_contract ${CONTRACTOR_LIBRARIES} osrm_update)
target_link_libraries(osrm_extract ${EXTRACTOR_LIBRARIES})
target_link_libraries(osrm_partition ${PARTITIONER_LIBRARIES})
target_link_libraries(osrm_customize ${CUSTOMIZER_LIBRARIES} osrm_update)
target_link_libraries(osrm_store ${STORAGE_LIBRARIES})

# BUILD_COMPONENTS
add_executable(osrm-components src/tools/components.cpp $<TARGET_OBJECTS:UTIL>)
target_link_libraries(osrm-components ${TBB_LIBRARIES} ${BOOST_BASE_LIBRARIES})
install(TARGETS osrm-components DESTINATION bin)

if(BUILD_TOOLS)
  message(STATUS "Activating OSRM internal tools")
  add_executable(osrm-io-benchmark src/tools/io-benchmark.cpp $<TARGET_OBJECTS:UTIL>)
  target_link_libraries(osrm-io-benchmark ${BOOST_BASE_LIBRARIES})

  install(TARGETS osrm-io-benchmark DESTINATION bin)

  find_package(Shapefile) # package libshp-dev
  if(SHAPEFILE_FOUND AND (Boost_VERSION VERSION_GREATER 106000 OR ENABLE_MASON))
    add_executable(osrm-extract-conditionals src/tools/extract-conditionals.cpp $<TARGET_OBJECTS:UTIL>)
    target_include_directories(osrm-extract-conditionals PRIVATE ${LIBSHAPEFILE_INCLUDE_DIR})
    target_link_libraries(osrm-extract-conditionals ${OSMIUM_LIBRARIES} ${BOOST_BASE_LIBRARIES} ${Boost_PROGRAM_OPTIONS_LIBRARY}
      ${LIBSHAPEFILE_LIBRARY} ${BZIP2_LIBRARIES} ${ZLIB_LIBRARY} ${EXPAT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    install(TARGETS osrm-extract-conditionals DESTINATION bin)
  endif()
endif()

if (ENABLE_ASSERTIONS)
  message(STATUS "Enabling assertions")
  add_definitions(-DBOOST_ENABLE_ASSERT_HANDLER)
endif()

# Add RPATH info to executables so that when they are run after being installed
# (i.e., from /usr/local/bin/) the linker can find library dependencies. For
# more info see http://www.cmake.org/Wiki/CMake_RPATH_handling
set_property(TARGET osrm-extract PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-partition PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-contract PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-datastore PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-routed PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

file(GLOB VariantGlob third_party/variant/include/mapbox/*.hpp)
file(GLOB LibraryGlob include/osrm/*.hpp)
file(GLOB ParametersGlob include/engine/api/*_parameters.hpp)
set(EngineHeader include/engine/status.hpp include/engine/engine_config.hpp include/engine/hint.hpp include/engine/bearing.hpp include/engine/phantom_node.hpp)
set(UtilHeader include/util/coordinate.hpp include/util/json_container.hpp include/util/typedefs.hpp include/util/strong_typedef.hpp include/util/exception.hpp)
set(ExtractorHeader include/extractor/extractor.hpp include/extractor/extractor_config.hpp include/extractor/travel_mode.hpp)
set(PartitionerHeader include/partition/partitioner.hpp include/partition/partition_config.hpp)
set(ContractorHeader include/contractor/contractor.hpp include/contractor/contractor_config.hpp)
set(StorageHeader include/storage/storage.hpp include/storage/storage_config.hpp)
install(FILES ${EngineHeader} DESTINATION include/osrm/engine)
install(FILES ${UtilHeader} DESTINATION include/osrm/util)
install(FILES ${StorageHeader} DESTINATION include/osrm/storage)
install(FILES ${ExtractorHeader} DESTINATION include/osrm/extractor)
install(FILES ${PartitionerHeader} DESTINATION include/osrm/partition)
install(FILES ${ContractorHeader} DESTINATION include/osrm/contractor)
install(FILES ${LibraryGlob} DESTINATION include/osrm)
install(FILES ${ParametersGlob} DESTINATION include/osrm/engine/api)
install(FILES ${VariantGlob} DESTINATION include/mapbox)
install(TARGETS osrm-extract DESTINATION bin)
install(TARGETS osrm-partition DESTINATION bin)
install(TARGETS osrm-contract DESTINATION bin)
install(TARGETS osrm-datastore DESTINATION bin)
install(TARGETS osrm-routed DESTINATION bin)
install(TARGETS osrm DESTINATION lib)
install(TARGETS osrm_extract DESTINATION lib)
install(TARGETS osrm_partition DESTINATION lib)
install(TARGETS osrm_update DESTINATION lib)
install(TARGETS osrm_contract DESTINATION lib)
install(TARGETS osrm_store DESTINATION lib)


# Install profiles and support library to /usr/local/share/osrm/profiles by default
set(DefaultProfilesDir profiles)
install(DIRECTORY ${DefaultProfilesDir} DESTINATION share/osrm)

# Setup exporting variables for pkgconfig and subproject
#

if(BUILD_PACKAGE)
  include(CPackConfig)
  include(CPack)
endif()

function(JOIN VALUES GLUE OUTPUT)
  string (REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

JOIN("${OSRM_DEFINES}" " " TMP_OSRM_DEFINES)
set(LibOSRM_CXXFLAGS "${OSRM_CXXFLAGS} ${TMP_OSRM_DEFINES}")
set(LibOSRM_LDFLAGS "${OSRM_LDFLAGS}")

if(BUILD_AS_SUBPROJECT)
  set(LibOSRM_CXXFLAGS "${LibOSRM_CXXFLAGS}" PARENT_SCOPE)
  set(LibOSRM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
  set(LibOSRM_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
  set(LibOSRM_LIBRARIES "osrm" PARENT_SCOPE)
  set(LibOSRM_DEPENDENT_LIBRARIES "${ENGINE_LIBRARIES}" PARENT_SCOPE)
  set(LibOSRM_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
                           "${CMAKE_CURRENT_SOURCE_DIR}/include/osrm"
                           "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
                           "${DEPENDENCIES_INCLUDE_DIRS}" PARENT_SCOPE)
  set(LibOSRM_LIBRARY_DIRS "${LibOSRM_LIBRARY_DIR}" PARENT_SCOPE)
endif()

# pkgconfig defines
set(PKGCONFIG_OSRM_CXXFLAGS "${LibOSRM_CXXFLAGS}")
set(PKGCONFIG_OSRM_LDFLAGS "${LibOSRM_LDFLAGS}")
set(PKGCONFIG_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib")
set(PKGCONFIG_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")

list(APPEND DEPENDENCIES_INCLUDE_DIRS "${PKGCONFIG_INCLUDE_DIR}")
list(APPEND DEPENDENCIES_INCLUDE_DIRS "${PKGCONFIG_INCLUDE_DIR}/osrm")
JOIN("-I${DEPENDENCIES_INCLUDE_DIRS}" " -I" PKGCONFIG_OSRM_INCLUDE_FLAGS)
JOIN("${ENGINE_LIBRARIES}" " " PKGCONFIG_OSRM_DEPENDENT_LIBRARIES)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkgconfig.in libosrm.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/libosrm.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)


# Modular build system: each directory registered here provides its own CMakeLists.txt
add_subdirectory(unit_tests)
add_subdirectory(src/benchmarks)

if (ENABLE_NODE_BINDINGS)
  add_subdirectory(src/nodejs)
endif()


if (ENABLE_FUZZING)
  # Requires libosrm being built with sanitizers; make configurable and default to ubsan
  set(FUZZ_SANITIZER "undefined" CACHE STRING "Sanitizer to be used for Fuzz testing")
  set_property(CACHE FUZZ_SANITIZER PROPERTY STRINGS "undefined" "integer" "address" "memory" "thread" "leak")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-coverage=edge,indirect-calls,8bit-counters -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(OSRM_LDFLAGS "${OSRM_LDFLAGS} -fsanitize=address")

  message(STATUS "Using -fsanitize=${FUZZ_SANITIZER} for Fuzz testing")

  add_subdirectory(fuzz)
endif ()


# add headers sanity check target that includes all headers independently
# make sure we have all deps for the nodejs sub project's includes (nan, node)
if (ENABLE_NODE_BINDINGS)
  set(check_headers_dir "${PROJECT_BINARY_DIR}/check-headers")
  file(GLOB_RECURSE headers_to_check
    ${PROJECT_BINARY_DIR}/*.hpp
    ${PROJECT_SOURCE_DIR}/include/*.hpp)
  foreach(header ${headers_to_check})
    get_filename_component(filename ${header} NAME_WE)
    set(filename "${check_headers_dir}/${filename}.cpp")
    if (NOT EXISTS ${filename})
      file(WRITE ${filename} "#include \"${header}\"\n")
    endif()
    list(APPEND sources ${filename})
  endforeach()
  add_library(check-headers STATIC EXCLUDE_FROM_ALL ${sources})
  set_target_properties(check-headers PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${check_headers_dir})
endif()
