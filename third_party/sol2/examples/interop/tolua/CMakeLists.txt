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

# # # sol2 interop Examples - tolua

find_package(ToLuappBuild REQUIRED)

function(make_tolua_interop_example target_library is_single)
	set(example_name tolua_interop_example)
	if (is_single)
		set(example_name "${example_name}.single")
	endif()
	add_executable(${example_name} tolua.cpp)
	target_link_libraries(${example_name} PUBLIC ${LUA_LIBRARIES} PRIVATE ${TOLUAPP_LIBRARIES} ${target_library})
	if (CMAKE_DL_LIBS)
		target_link_libraries(${example_name} PRIVATE ${CMAKE_DL_LIBS})
	endif()
	if (MSVC)
		target_compile_options(${example_name} PRIVATE /W1)
	else()
		target_compile_options(${example_name} PRIVATE -w)
	endif()
	if (TESTS_EXAMPLES)
		add_test(NAME ${example_name} COMMAND ${example_name})
	endif()
endfunction()

make_tolua_interop_example(sol2 FALSE)
if (SOL2_SINGLE_FOUND AND INTEROP_EXAMPLES_SINGLE)
	make_tolua_interop_example(sol2_single TRUE)
endif()