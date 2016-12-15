#include <boost/assert.hpp>

#ifdef OSRM_HAS_STACKTRACE
// From vendored third_party/stacktrace for now; will be included in Boost in the future
#include <boost/stacktrace.hpp>
#endif

#include <exception>
#include <iostream>

namespace
{
// We hard-abort on assertion violations.
void assertion_failed_msg_helper(
    char const *expr, char const *msg, char const *function, char const *file, long line)
{
    std::cerr << "[assert] " << file << ":" << line << "\nin: " << function << ": " << expr << "\n"
              << msg
#ifdef OSRM_HAS_STACKTRACE
              << "\nBacktrace:\n"
              << boost::stacktrace::stacktrace() << "\n";
#else
              << "\n";
#endif

    std::terminate();
}
}

// Boost.Assert only declares the following two functions and let's us define them here.
namespace boost
{
void assertion_failed(char const *expr, char const *function, char const *file, long line)
{
    ::assertion_failed_msg_helper(expr, "", function, file, line);
}
void assertion_failed_msg(
    char const *expr, char const *msg, char const *function, char const *file, long line)
{
    ::assertion_failed_msg_helper(expr, msg, function, file, line);
}
}
