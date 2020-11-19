# # # # sol2
# The MIT License (MIT)
# 
# Copyright (c) 2013-2018 Rapptz, ThePhD, and contributors
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# # # sol2 Examples

if (DYNAMIC_LOADING_EXAMPLES)
	# # require_from_dll example
	# just add the subdirectory
	add_subdirectory(require_dll_example)
endif(DYNAMIC_LOADING_EXAMPLES)

if (INTEROP_EXAMPLES)
	# # interop examples
	add_subdirectory(interop/kaguya)
	add_subdirectory(interop/tolua)
	add_subdirectory(interop/LuaBridge)
	add_subdirectory(interop/luwra)
endif(INTEROP_EXAMPLES)

# # single-source compilable examples

file(GLOB EXAMPLES_SRC *.cpp tutorials/quick_n_dirty/*.cpp docs/*.cpp)
source_group(examples FILES ${EXAMPLES_SRC})

function (MAKE_EXAMPLE example_source_file is_single)
	get_filename_component(example_name ${example_source_file} NAME_WE)
	file(RELATIVE_PATH example_source_file_relative ${CMAKE_SOURCE_DIR} ${example_source_file})
	get_filename_component(example_output_relative_dir ${example_source_file_relative} DIRECTORY)
	file(TO_CMAKE_PATH "${example_output_relative_dir}" example_output_relative_dir_name)
	STRING(REGEX REPLACE "/" "." example_output_relative_dir_name "${example_output_relative_dir}")
	if (is_single)
		set(example_name "${example_name}.single")
	endif()
	if (example_output_relative_dir_name STREQUAL "")
		set(example_output_name "${example_name}")
	else()
		set(example_output_name "${example_output_relative_dir_name}.${example_name}")
	endif()

	add_executable(${example_name} ${example_source_file} assert.hpp)
	set_target_properties(${example_name} 
		PROPERTIES
		OUTPUT_NAME "${example_output_name}")
	if (is_single)
		target_link_libraries(${example_name} sol2_single)
	else()
		target_link_libraries(${example_name} sol2)
	endif()
	target_compile_features(${example_name} 
		PRIVATE ${CXX_FEATURES})
	target_link_libraries(${example_name} ${LUA_LIBRARIES})
	if(CMAKE_DL_LIBS)
		target_link_libraries(${example_name} ${CMAKE_DL_LIBS})
	endif()
	if (CI)
		target_compile_definitions(${example_name} 
			PRIVATE SOL2_CI)
	endif()
	if (MSVC)

	else()
		target_compile_options(${example_name}
			PRIVATE -Wno-noexcept-type)
	endif()
	
	if (TESTS_EXAMPLES)
		if ((NOT is_single) OR (is_single AND TESTS_SINGLE))
			add_test(NAME ${example_output_name} COMMAND ${example_name})
		endif()
	endif()
	install(TARGETS ${example_name} RUNTIME DESTINATION bin)
endfunction(MAKE_EXAMPLE)

if (EXAMPLES)
	foreach(example_source_file ${EXAMPLES_SRC})
		MAKE_EXAMPLE(${example_source_file} FALSE)
	endforeach()

	if (SOL2_SINGLE_FOUND AND EXAMPLES_SINGLE)
		foreach(example_source_file ${EXAMPLES_SRC})
			MAKE_EXAMPLE(${example_source_file} TRUE)
		endforeach()
	endif()
endif()
