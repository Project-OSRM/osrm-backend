#ifndef OSRM_EXTRACTOR_HEAP_PAGES_HPP
#define OSRM_EXTRACTOR_HEAP_PAGES_HPP

#include <cstdlib>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__GLIBC__)
#include <malloc.h>
#endif

namespace osrm::extractor
{

inline void releaseUnusedHeapPages() noexcept
{
#if defined(__APPLE__)
    malloc_zone_pressure_relief(nullptr, 0);
#elif defined(__GLIBC__)
    malloc_trim(0);
#endif
}

} // namespace osrm::extractor

#endif
