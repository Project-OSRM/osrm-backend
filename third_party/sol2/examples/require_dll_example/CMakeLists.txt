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

# # # sol2 Examples - require_from_dll

# # Reusable function to call for single target
# # Also hides variables from directory/global scope
function(make_require_from_dll_example target_lib is_single)
	# define sources
	set(my_object_sources my_object.cpp my_object.hpp my_object_api.hpp)
	set(require_from_dll_sources require_from_dll.cpp)

	# define names
	set(example_lib_name my_object)
	set(example_name require_from_dll)
	if (is_single)
		set(example_lib_name "${example_lib_name}.single")
		set(example_name "${example_name}.single")
	endif()

	# is the lua library a shared or static library?
	list(GET LUA_LIBRARIES 0 lua_lib_target)
	get_target_property(lua_lib_type ${lua_lib_target} TYPE)

	# add library target my_object for the require_from_dll program
	add_library(${example_lib_name} SHARED ${my_object_sources})
	set_target_properties(${example_lib_name} PROPERTIES
		PREFIX "")
	target_include_directories(${example_lib_name} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
	target_compile_features(${example_lib_name} PRIVATE ${CXX_FEATURES})
	target_compile_definitions(${example_lib_name} PUBLIC MY_OBJECT_DLL PRIVATE MY_OBJECT_BUILD)
	if(CMAKE_DL_LIBS)
		target_link_libraries(${example_lib_name} PUBLIC ${CMAKE_DL_LIBS})
	endif()
	if (CI)
		target_compile_definitions(${example_lib_name} PRIVATE SOL2_CI)
	endif()
	if (NOT MSVC)
		target_compile_options(${example_lib_name} PRIVATE -Wno-noexcept-type)
		if (lua_lib_type MATCHES "STATIC")
			# ensure that the whole archive is input into the linker
			# this ensure static builds are included properly
			target_link_libraries(${example_lib_name} PRIVATE
				-Wl,-whole-archive ${LUA_LIBRARIES} -Wl,-no-whole-archive)
		else()
			target_link_libraries(${example_lib_name} PRIVATE ${LUA_LIBRARIES})
		endif()
	else()
		target_link_libraries(${example_lib_name} PRIVATE ${LUA_LIBRARIES})
	endif()
	if (IS_X86)
		target_compile_options(${example_lib_name} BEFORE PRIVATE -m32)
	endif()
	target_link_libraries(${example_lib_name} PRIVATE ${target_lib})
	target_include_directories(${example_lib_name} PUBLIC "${LUA_INCLUDE_DIRS}")

	# add executable target that represents require_from_dll program
	add_executable(${example_name} ${require_from_dll_sources})
	target_compile_features(${example_name} PRIVATE ${CXX_FEATURES})
	if(CMAKE_DL_LIBS)
		target_link_libraries(${example_name} PRIVATE ${CMAKE_DL_LIBS})
	endif()
	if (CI)
		target_compile_definitions(${example_name} PRIVATE SOL2_CI)
	endif()
	if (NOT MSVC)
		target_compile_options(${example_name} PRIVATE -Wno-noexcept-type)
	endif()
	target_link_libraries(${example_name} PRIVATE my_object ${target_lib})
	# avoid multiply defined references due to linking in the same static library
	# twice over, and get "multiple definition" errors during linking
	if (NOT lua_lib_type MATCHES "STATIC")
		target_link_libraries(${example_name} PRIVATE ${LUA_LIBRARIES})
	endif()
	target_include_directories(${example_name} PRIVATE ${LUA_INCLUDE_DIRS})

	if (TESTS_DYNAMIC_LOADING_EXAMPLES)
		if ((NOT is_single) OR (is_single AND DYNAMIC_LOADING_EXAMPLES_SINGLE))
			get_target_property(example_working_dir ${example_name} RUNTIME_OUTPUT_DIRECTORY)
			add_test(NAME ${example_name} COMMAND ${example_name} WORKING_DIRECTORY "${example_working_dir}")
		endif()
	endif()
endfunction()

make_require_from_dll_example(sol2 FALSE)
if (SOL2_SINGLE_FOUND AND DYNAMIC_LOADING_EXAMPLES_SINGLE)
	make_require_from_dll_example(sol2_single TRUE)
endif()