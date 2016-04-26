cmake_minimum_required(VERSION 2.8.8)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR AND NOT MSVC_IDE)
  message(FATAL_ERROR "In-source builds are not allowed.
Please create a directory and run cmake from there, passing the path to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'. Please delete them.")
endif()

project(OSRM C CXX)
set(OSRM_VERSION_MAJOR 5)
set(OSRM_VERSION_MINOR 1)
set(OSRM_VERSION_PATCH 0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CheckCXXCompilerFlag)
include(FindPackageHandleStandardArgs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(bitness 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(bitness 64)
  message(STATUS "Building on a 64 bit system")
else()
  message(WARNING "Building on a 32 bit system is unsupported")
endif()

if(WIN32 AND MSVC_VERSION LESS 1900)
  message(FATAL_ERROR "Building with Microsoft compiler needs Latest Visual Studio 2015 (Community or better)")
endif()

option(ENABLE_CCACHE "Speed up incremental rebuilds via ccache" ON)
option(ENABLE_JSON_LOGGING "Adds additional JSON debug logging to the response" OFF)
option(BUILD_TOOLS "Build OSRM tools" OFF)
option(BUILD_COMPONENTS "Build osrm-components" OFF)
option(ENABLE_ASSERTIONS OFF)
option(COVERAGE OFF)
option(SANITIZER OFF)

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}/include/)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/)

add_custom_target(FingerPrintConfigure ALL ${CMAKE_COMMAND}
  "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
  "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FingerPrint-Config.cmake"
  COMMENT "Configuring revision fingerprint"
  VERBATIM)

set(BOOST_COMPONENTS date_time filesystem iostreams program_options regex system thread)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/util/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/util/version.hpp
)
file(GLOB UtilGlob src/util/*.cpp src/util/*/*.cpp)
file(GLOB ExtractorGlob src/extractor/*.cpp src/extractor/*/*.cpp)
file(GLOB ContractorGlob src/contractor/*.cpp)
file(GLOB StorageGlob src/storage/*.cpp)
file(GLOB ServerGlob src/server/*.cpp src/server/**/*.cpp)
file(GLOB EngineGlob src/engine/*.cpp src/engine/**/*.cpp)

add_library(UTIL OBJECT ${UtilGlob})
add_library(EXTRACTOR OBJECT ${ExtractorGlob})
add_library(CONTRACTOR OBJECT ${ContractorGlob})
add_library(STORAGE OBJECT ${StorageGlob})
add_library(ENGINE OBJECT ${EngineGlob})
add_library(SERVER OBJECT ${ServerGlob})

add_dependencies(UTIL FingerPrintConfigure)
set_target_properties(UTIL PROPERTIES LINKER_LANGUAGE CXX)

add_executable(osrm-extract src/tools/extract.cpp)
add_executable(osrm-contract src/tools/contract.cpp)
add_executable(osrm-routed src/tools/routed.cpp $<TARGET_OBJECTS:SERVER> $<TARGET_OBJECTS:UTIL>)
add_executable(osrm-datastore src/tools/store.cpp $<TARGET_OBJECTS:UTIL>)
add_library(osrm src/osrm/osrm.cpp $<TARGET_OBJECTS:ENGINE> $<TARGET_OBJECTS:UTIL> $<TARGET_OBJECTS:STORAGE>)
add_library(osrm_extract $<TARGET_OBJECTS:EXTRACTOR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_contract $<TARGET_OBJECTS:CONTRACTOR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_store $<TARGET_OBJECTS:STORAGE> $<TARGET_OBJECTS:UTIL>)

# Check the release mode
if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Configuring OSRM in debug mode")
  set(ENABLE_ASSERTIONS ON)
  if(NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline -fno-omit-frame-pointer")

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -ggdb")
  endif()

  endif()
endif()
if(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Configuring OSRM in release mode")
  # Check if LTO is available
  check_cxx_compiler_flag("-flto" LTO_AVAILABLE)
  if(LTO_AVAILABLE)
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
    else()
      message(STATUS "LTO broken")
      set(CMAKE_CXX_FLAGS "${OLD_CXX_FLAGS}")
    endif()

    # Since gcc 4.9 the LTO format is non-standart ('slim'), so we need to use the build-in tools
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND
        NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.9.0" AND NOT MINGW)
      message(STATUS "Using gcc specific binutils for LTO.")
      set(CMAKE_AR     "/usr/bin/gcc-ar")
      set(CMAKE_RANLIB "/usr/bin/gcc-ranlib")
    endif()

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.9.0")
      message(STATUS "Disabling LTO on GCC < 4.9.0 since it is broken, see: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57038")
      set(CMAKE_CXX_FLAGS "${OLD_CXX_FLAGS}")
    endif()
  endif()
endif()

set(MAYBE_COVERAGE_LIBRARIES "")
if (COVERAGE)
  if (NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    message(ERROR "COVERAGE=ON only make sense with a Debug build")
  endif()
  message(INFO "Enabling coverage")
  set(MAYBE_COVERAGE_LIBRARIES "gcov")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs")
endif()
if (SANITIZER)
  if (NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    message(ERROR "SANITIZER=ON only make sense with a Debug build")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

# Configuring compilers
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wuninitialized -Wunreachable-code -Wstrict-overflow=2 -D_FORTIFY_SOURCE=2 -fPIC -fcolor-diagnostics")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  set(COLOR_FLAG "-fdiagnostics-color=auto")
  check_cxx_compiler_flag("-fdiagnostics-color=auto" HAS_COLOR_FLAG)
  if(NOT HAS_COLOR_FLAG)
    set(COLOR_FLAG "")
  endif()
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wuninitialized -Wunreachable-code -Wstrict-overflow=1 -D_FORTIFY_SOURCE=2 ${COLOR_FLAG} -fPIC")
  if(WIN32) # using mingw
    add_definitions(-DWIN32)
    set(OPTIONAL_SOCKET_LIBS ws2_32 wsock32)
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
  # using Intel C++
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-intel -wd10237 -Wall -ipo -fPIC")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
  # using Visual Studio C++
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} date_time chrono zlib)
  add_definitions(-DBOOST_LIB_DIAGNOSTIC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-DNOMINMAX) # avoid min and max macros that can break compilation
  add_definitions(-D_USE_MATH_DEFINES) #needed for M_PI with cmath.h
  add_definitions(-D_WIN32_WINNT=0x0501)
  add_definitions(-DXML_STATIC)
  find_library(ws2_32_LIBRARY_PATH ws2_32)
  target_link_libraries(osrm-extract wsock32 ws2_32)
endif()

# Configuring linker
execute_process(COMMAND ${CMAKE_CXX_COMPILER} "-Wl,--version" ERROR_QUIET OUTPUT_VARIABLE LINKER_VERSION)
# For ld.gold and ld.bfs (the GNU linkers) we optimize hard
if("${LINKER_VERSION}" MATCHES "GNU gold" OR "${LINKER_VERSION}" MATCHES "GNU ld")
  message(STATUS "Setting linker optimizations")
  if(NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
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
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

# Activate C++11
if(NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")
endif()

# Configuring other platform dependencies
if(APPLE)
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
  set(MAYBE_RT_LIBRARY rt)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium/cmake")
set(OSMIUM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium/include")
find_package(Osmium REQUIRED COMPONENTS io)
include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS})


find_package(Boost 1.49.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
if(NOT WIN32)
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()
add_definitions(-DBOOST_SPIRIT_USE_PHOENIX_V3)
add_definitions(-DBOOST_RESULT_OF_USE_DECLTYPE)
add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

find_package(Threads REQUIRED)

find_package(TBB REQUIRED)
include_directories(SYSTEM ${TBB_INCLUDE_DIR})
if(WIN32 AND CMAKE_BUILD_TYPE MATCHES Debug)
  set(TBB_LIBRARIES ${TBB_DEBUG_LIBRARIES})
endif()

find_package(Luabind REQUIRED)
include(check_luabind)
include_directories(SYSTEM ${LUABIND_INCLUDE_DIR})

set(USED_LUA_LIBRARIES ${LUA_LIBRARY})
if(LUAJIT_FOUND)
  set(USED_LUA_LIBRARIES, LUAJIT_LIBRARIES)
endif()
include_directories(SYSTEM ${LUA_INCLUDE_DIR})

find_package(EXPAT REQUIRED)
include_directories(SYSTEM ${EXPAT_INCLUDE_DIRS})

find_package(STXXL REQUIRED)
include_directories(SYSTEM ${STXXL_INCLUDE_DIR})

set(OpenMP_FIND_QUIETLY ON)
find_package(OpenMP)
if(OPENMP_FOUND)
  message(STATUS "OpenMP support found. Linking just in case for stxxl")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

find_package(BZip2 REQUIRED)
include_directories(SYSTEM ${BZIP_INCLUDE_DIRS})

find_package(ZLIB REQUIRED)
include_directories(SYSTEM ${ZLIB_INCLUDE_DIRS})

if (ENABLE_JSON_LOGGING)
  message(STATUS "Enabling json logging")
  add_definitions(-DENABLE_JSON_LOGGING)
endif()

# Binaries
target_link_libraries(osrm-datastore osrm_store ${Boost_LIBRARIES})
target_link_libraries(osrm-extract osrm_extract ${Boost_LIBRARIES})
target_link_libraries(osrm-contract osrm_contract ${Boost_LIBRARIES})
target_link_libraries(osrm-routed osrm ${Boost_LIBRARIES} ${OPTIONAL_SOCKET_LIBS} ${ZLIB_LIBRARY})

set(EXTRACTOR_LIBRARIES
    ${BZIP2_LIBRARIES}
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXPAT_LIBRARIES}
    ${LUABIND_LIBRARY}
    ${USED_LUA_LIBRARIES}
    ${OSMIUM_LIBRARIES}
    ${STXXL_LIBRARY}
    ${TBB_LIBRARIES}
    ${ZLIB_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(CONTRACTOR_LIBRARIES
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LUABIND_LIBRARY}
    ${USED_LUA_LIBRARIES}
    ${STXXL_LIBRARY}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(ENGINE_LIBRARIES
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${STXXL_LIBRARY}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(STORAGE_LIBRARIES
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(UTIL_LIBRARIES
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${STXXL_LIBRARY}
    ${TBB_LIBRARIES}
    ${MAYBE_COVERAGE_LIBRARIES})
# Libraries
target_link_libraries(osrm ${ENGINE_LIBRARIES})
target_link_libraries(osrm_contract ${CONTRACTOR_LIBRARIES})
target_link_libraries(osrm_extract ${EXTRACTOR_LIBRARIES})
target_link_libraries(osrm_store ${STORAGE_LIBRARIES})

if(BUILD_COMPONENTS)
  find_package(GDAL)
  if(GDAL_FOUND)
    add_executable(osrm-components src/tools/components.cpp $<TARGET_OBJECTS:UTIL>)
    target_link_libraries(osrm-components ${TBB_LIBRARIES})
    include_directories(SYSTEM ${GDAL_INCLUDE_DIR})
    target_link_libraries(osrm-components ${GDAL_LIBRARIES} ${Boost_LIBRARIES})
    install(TARGETS osrm-components DESTINATION bin)
  else()
    message(WARNING "libgdal and/or development headers not found")
  endif()
endif()

if(BUILD_TOOLS)
  message(STATUS "Activating OSRM internal tools")
  add_executable(osrm-io-benchmark src/tools/io-benchmark.cpp $<TARGET_OBJECTS:UTIL>)
  target_link_libraries(osrm-io-benchmark ${Boost_LIBRARIES})
  add_executable(osrm-unlock-all src/tools/unlock_all_mutexes.cpp $<TARGET_OBJECTS:UTIL>)
  target_link_libraries(osrm-unlock-all ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  if(UNIX AND NOT APPLE)
    target_link_libraries(osrm-unlock-all rt)
  endif()
  add_executable(osrm-springclean src/tools/springclean.cpp $<TARGET_OBJECTS:UTIL>)
  target_link_libraries(osrm-springclean ${Boost_LIBRARIES})

  install(TARGETS osrm-io-benchmark DESTINATION bin)
  install(TARGETS osrm-unlock-all DESTINATION bin)
  install(TARGETS osrm-springclean DESTINATION bin)
endif()

if (ENABLE_ASSERTIONS)
  message(STATUS "Enabling assertions")
  add_definitions(-DBOOST_ENABLE_ASSERT_HANDLER)
endif()

# Add RPATH info to executables so that when they are run after being installed
# (i.e., from /usr/local/bin/) the linker can find library dependencies. For
# more info see http://www.cmake.org/Wiki/CMake_RPATH_handling
set_property(TARGET osrm-extract PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-contract PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-datastore PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-routed PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

file(GLOB VariantGlob third_party/variant/*.hpp)
file(GLOB LibraryGlob include/osrm/*.hpp)
file(GLOB ParametersGlob include/engine/api/*_parameters.hpp)
set(EngineHeader include/engine/status.hpp include/engine/engine_config.hpp include/engine/hint.hpp include/engine/bearing.hpp include/engine/phantom_node.hpp)
set(UtilHeader include/util/coordinate.hpp include/util/json_container.hpp include/util/typedefs.hpp include/util/strong_typedef.hpp)
set(ExtractorHeader include/extractor/extractor.hpp include/extractor/extractor_config.hpp include/extractor/travel_mode.hpp)
set(ContractorHeader include/contractor/contractor.hpp include/contractor/contractor_config.hpp)
set(StorageHeader include/storage/storage.hpp include/storage/storage_config.hpp)
install(FILES ${EngineHeader} DESTINATION include/osrm/engine)
install(FILES ${UtilHeader} DESTINATION include/osrm/util)
install(FILES ${StorageHeader} DESTINATION include/osrm/storage)
install(FILES ${ExtractorHeader} DESTINATION include/osrm/extractor)
install(FILES ${ContractorHeader} DESTINATION include/osrm/contractor)
install(FILES ${LibraryGlob} DESTINATION include/osrm)
install(FILES ${ParametersGlob} DESTINATION include/osrm/engine/api)
install(FILES ${VariantGlob} DESTINATION include/variant)
install(TARGETS osrm-extract DESTINATION bin)
install(TARGETS osrm-contract DESTINATION bin)
install(TARGETS osrm-datastore DESTINATION bin)
install(TARGETS osrm-routed DESTINATION bin)
install(TARGETS osrm DESTINATION lib)
install(TARGETS osrm_extract DESTINATION lib)
install(TARGETS osrm_contract DESTINATION lib)
install(TARGETS osrm_store DESTINATION lib)

list(GET ENGINE_LIBRARIES 1 ENGINE_LIBRARY_FIRST)
foreach(lib ${ENGINE_LIBRARIES})
  get_filename_component(ENGINE_LIBRARY_PATH "${ENGINE_LIBRARY_FIRST}" PATH)
  get_filename_component(ENGINE_LIBRARY_NAME "${lib}" NAME_WE)
  string(REPLACE "lib" "" ENGINE_LIBRARY_NAME ${ENGINE_LIBRARY_NAME})
  string(REPLACE "-l" "" ENGINE_LIBRARY_NAME ${ENGINE_LIBRARY_NAME})
  set(ENGINE_LIBRARY_LISTING "${ENGINE_LIBRARY_LISTING} -L${ENGINE_LIBRARY_PATH} -l${ENGINE_LIBRARY_NAME}")
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkgconfig.in libosrm.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/libosrm.pc DESTINATION lib/pkgconfig)

if(BUILD_DEBIAN_PACKAGE)
  include(CPackDebianConfig)
  include(CPack)
endif()

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
