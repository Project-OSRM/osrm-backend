INCLUDE (CheckCXXSourceCompiles)
unset(STXXL_WORKS CACHE)
set (STXXL_CHECK_SRC "#include <stxxl/vector>\n int main() { stxxl::vector<int> vec; return 0;}")
set (CMAKE_TRY_COMPILE_CONFIGURATION ${CMAKE_BUILD_TYPE})
set (CMAKE_REQUIRED_INCLUDES "${STXXL_INCLUDE_DIR}")
set (CMAKE_REQUIRED_LIBRARIES "${STXXL_LIBRARY}")

CHECK_CXX_SOURCE_COMPILES("${STXXL_CHECK_SRC}" STXXL_WORKS)

if(STXXL_WORKS)
  message(STATUS "STXXL can be used without linking against libgomp")
else()
  unset(STXXL_WORKS)
  message(STATUS "STXXL failed without libgomp, retrying ..")
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} -lgomp)

  CHECK_CXX_SOURCE_COMPILES("${STXXL_CHECK_SRC}" STXXL_WORKS)

  if (STXXL_WORKS)
    target_link_libraries(osrm-extract gomp)
  else()
    message(FATAL "STXXL failed failed, libgomp missing?")
  endif()
endif()
