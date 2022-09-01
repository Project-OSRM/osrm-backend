#include <boost/assert.hpp>

#include <exception>
#include <iostream>
#include <thread>

namespace
{
// We hard-abort on assertion violations.
[[noreturn]] void assertion_failed_msg_helper(
    char const *expr, char const *msg, char const *function, char const *file, long line)
{
    const auto tid = std::this_thread::get_id();

    std::cerr << "[assert][" << tid << "] " << file << ":" << line << "\nin: " << function << ": "
              << expr << "\n"
              << msg;

    std::terminate();
}
} // namespace

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
} // namespace boost
