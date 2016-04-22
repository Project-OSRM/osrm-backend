#include "util/assert.hpp"

#include <sstream>

namespace
{
// We throw to guarantee for stack-unwinding and therefore our destructors being called.
void assertion_failed_msg_helper(
    char const *expr, char const *msg, char const *function, char const *file, long line)
{
    std::ostringstream fmt;
    fmt << file << ":" << line << "\nin: " << function << ": " << expr << "\n" << msg;
    throw osrm::util::assertionError{fmt.str().c_str()};
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
