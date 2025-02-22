#ifndef EXCEPTION_UTILS_HPP
#define EXCEPTION_UTILS_HPP

#include <cstring>
#include <string>

// Helper macros, don't use these ones
// STRIP the OSRM_PROJECT_DIR from the front of a filename.  Expected to come
// from CMake's CURRENT_SOURCE_DIR, which doesn't have a trailing /, hence the +1
#define PROJECT_RELATIVE_PATH_(x) std::string(x).substr(strlen(OSRM_PROJECT_DIR) + 1)
// Return the path of a file, relative to the OSRM_PROJECT_DIR
#define OSRM_SOURCE_FILE_ PROJECT_RELATIVE_PATH_(__FILE__)

// This is the macro to use
#define SOURCE_REF (OSRM_SOURCE_FILE_ + ":" + std::to_string(__LINE__))

#endif // EXCEPTION_UTILS_HPP
