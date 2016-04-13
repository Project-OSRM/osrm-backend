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
  add_executable(driver driver.cc $<TARGET_OBJECTS:UTIL> $<TARGET_OBJECTS:SERVER>)
  target_link_libraries(driver Fuzzer osrm)

  add_custom_target(fuzz
	  DEPENDS driver
	  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	  COMMAND ${CMAKE_COMMAND} -E make_directory corpus
	  COMMAND driver -jobs=4 -workers=4 -max_len=4096 corpus
	  COMMENT "Fuzzing libosrm" VERBATIM)
endif ()
