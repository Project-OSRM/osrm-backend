#ifndef MEMINFO_HPP
#define MEMINFO_HPP

#include "util/log.hpp"

#ifndef _WIN32
#include <sys/resource.h>
#endif

namespace osrm
{
namespace util
{

inline void DumpMemoryStats()
{
#ifndef _WIN32
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
#ifdef __linux__
    // Under linux, ru.maxrss is in kb
    util::Log() << "RAM: peak bytes used: " << usage.ru_maxrss * 1024;
#else  // __linux__
    // Under BSD systems (OSX), it's in bytes
    util::Log() << "RAM: peak bytes used: " << usage.ru_maxrss;
#endif // __linux__
#else  // _WIN32
    util::Log() << "RAM: peak bytes used: <not implemented on Windows>";
#endif // _WIN32
}
} // namespace util
} // namespace osrm

#endif
