#ifndef OSRM_FUZZ_UTIL_HPP
#define OSRM_FUZZ_UTIL_HPP

#include <type_traits>

// Fakes observable side effects the compiler can not optimize away
template <typename T> inline void escape(T p)
{
    static_assert(std::is_pointer<T>::value, "");
    asm volatile("" : : "g"((void *)p) : "memory");
}

// Possibly reads and writes all the memory in your system
inline void clobber() { asm volatile("" : : : "memory"); }

#endif
