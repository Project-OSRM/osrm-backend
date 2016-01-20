#ifndef OSRM_ASSERT_HPP
#define OSRM_ASSERT_HPP

#include <boost/assert.hpp>

#include <stdexcept>

namespace osrm
{
namespace util
{
// Assertion type to be thrown for stack unwinding
struct assertionError final : std::logic_error
{
    assertionError(const char *msg) : std::logic_error{msg} {}
};
}
}

#endif
