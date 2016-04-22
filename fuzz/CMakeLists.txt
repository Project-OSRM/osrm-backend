# Fuzz testing using LLVM's libFuzzer.
#
# See:
#  - http://llvm.org/docs/LibFuzzer.html
#  - http://llvm.org/releases/3.8.0/docs/LibFuzzer.html
#
# TODO(daniel-j-h):
#  - make more user friendly, at the moment we require you to build and install libFuzzer.a
#  - pick up LLVM_ROOT
#  - build libFuzzer on the fly
#
# clang++ -std=c++11 -stdlib=libc++ -c -g -O2 ~/llvm/lib/Fuzzer/*.cpp -I~/llvm/lib/Fuzzer
# ar ruv libFuzzer.a Fuzzer*.o

if (ENABLE_FUZZING)

  include(ProcessorCount)
  ProcessorCount(nproc)

  macro(add_fuzz_target binary)
    add_executable(${binary} ${binary}.cc $<TARGET_OBJECTS:UTIL> $<TARGET_OBJECTS:SERVER>)
    target_link_libraries(${binary} Fuzzer osrm)
    target_include_directories(${binary} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    add_custom_target(fuzz-${binary}
	    DEPENDS ${binary}
	    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	    COMMAND ${CMAKE_COMMAND} -E make_directory "corpus/${binary}"
	    COMMAND ${binary} -use_traces=1 -jobs=${nproc} -workers=${nproc} -max_len=4096 "corpus/${binary}"
	    COMMENT "Fuzzing ${binary}" VERBATIM)
  endmacro ()


  set(ServerTargets
	  "match_parameters"
	  "nearest_parameters"
	  "route_parameters"
	  "table_parameters"
	  "tile_parameters"
	  "trip_parameters"
	  "url_parser"
	  "request_parser")

  foreach (target ${ServerTargets})
	  add_fuzz_target(${target})
  endforeach ()


  set(UtilTargets
	  "escape_json"
	  "uri_decode")

  foreach (target ${UtilTargets})
	  add_fuzz_target(${target})
  endforeach ()


endif ()
