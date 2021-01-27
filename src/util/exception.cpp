#include "util/exception.hpp"

// This function exists to 'anchor' the class, and stop the compiler from
// copying vtable and RTTI info into every object file that includes
// this header. (Caught by -Wweak-vtables under Clang.)

// More information from the LLVM Coding Standards:
// If a class is defined in a header file and has a vtable (either it has
// virtual methods or it derives from classes with virtual methods), it must
// always have at least one out-of-line virtual method in the class. Without
// this, the compiler will copy the vtable and RTTI into every .o file that
// #includes the header, bloating .o file sizes and increasing link times.

namespace osrm
{
namespace util
{

void exception::anchor() const {}
void RuntimeError::anchor() const {}
} // namespace util
} // namespace osrm
