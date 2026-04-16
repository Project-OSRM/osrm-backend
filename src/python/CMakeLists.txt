message(STATUS "Building Python bindings")

set(EXT_NAME "osrm_ext")

find_package(Python
  REQUIRED COMPONENTS Interpreter Development.Module
  OPTIONAL_COMPONENTS Development.SABIModule
)

# Find nanobind's cmake dir via the Python module
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NB_DIR
)
list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
find_package(nanobind CONFIG REQUIRED)

set(SRCS
  src/osrm_nb.cpp
  src/engineconfig_nb.cpp
  src/utility/osrm_utility.cpp
  src/utility/param_utility.cpp

  src/parameters/baseparameter_nb.cpp
  src/parameters/routeparameter_nb.cpp
  src/parameters/matchparameter_nb.cpp
  src/parameters/nearestparameter_nb.cpp
  src/parameters/tableparameter_nb.cpp
  src/parameters/tileparameter_nb.cpp
  src/parameters/tripparameter_nb.cpp

  src/types/optional_nb.cpp
  src/types/coordinate_nb.cpp
  src/types/jsoncontainer_nb.cpp
  src/types/approach_nb.cpp
  src/types/bearing_nb.cpp
)
nanobind_add_module(
  ${EXT_NAME}
  STABLE_ABI
  NB_STATIC
  ${SRCS}
)

# OSRM's root CMakeLists.txt enables CMAKE_INTERPROCEDURAL_OPTIMIZATION globally,
# which conflicts with nanobind's NB_MAKE_OPAQUE macros (ODR violations across TUs
# flagged as errors during LTO). Disable IPO for the extension module — nanobind's
# own LTO flag (passed to nanobind_add_module above) handles LTO for the binding.
set_target_properties(${EXT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION FALSE)
# Suppress warnings from nanobind headers included by our binding sources.
target_compile_options(${EXT_NAME} PRIVATE
  -Wno-suggest-destructor-override
  -Wno-redundant-decls
)
# Also suppress LTO warnings-as-errors at link time, since the osrm library
# is compiled with fat LTO objects and GCC's LTO linker plugin sees ODR
# differences between nanobind's type_caster and NB_MAKE_OPAQUE macros.
target_link_options(${EXT_NAME} PRIVATE -Wno-error)

# nanobind's internal sources trigger warnings under OSRM's strict -Werror flags
# (inherited via CMAKE_CXX_FLAGS). Suppress for the nanobind static library target.
set(_nb_suppress -Wno-error -Wno-suggest-destructor-override -Wno-redundant-decls)
if(TARGET nanobind-static-abi3)
  target_compile_options(nanobind-static-abi3 PRIVATE ${_nb_suppress})
elseif(TARGET nanobind-static)
  target_compile_options(nanobind-static PRIVATE ${_nb_suppress})
endif()

# Binding headers live in src/python/include/python/
target_include_directories(${EXT_NAME}
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link against the osrm library built by the parent project
target_link_libraries(${EXT_NAME}
  PRIVATE
    osrm
    Python::SABIModule
)

if (MSVC)
  target_compile_definitions(${EXT_NAME} PRIVATE BOOST_ALL_NO_LIB)
endif()

# Install the extension module into the osrm Python package
install(TARGETS ${EXT_NAME} LIBRARY DESTINATION osrm)

# Install OSRM executables and profiles into the wheel
set(OSRM_EXECUTABLES osrm-extract osrm-contract osrm-customize osrm-partition osrm-datastore osrm-routed osrm-components)
install(TARGETS ${OSRM_EXECUTABLES} DESTINATION osrm/bin)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/profiles DESTINATION osrm/share)

# Generate Python type stubs (.pyi) from the compiled extension module.
# Stubs are written to the source tree so they can be committed and used by
# documentation tools without compiling the extension.
nanobind_add_stub(
  ${EXT_NAME}_stub
  MODULE osrm_ext
  OUTPUT ${PROJECT_SOURCE_DIR}/src/python/osrm/osrm_ext.pyi
  PYTHON_PATH $<TARGET_FILE_DIR:${EXT_NAME}>
  DEPENDS ${EXT_NAME}
)
