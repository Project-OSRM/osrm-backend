#ifndef MEMINFO_HPP
#define MEMINFO_HPP

#include "util/log.hpp"

#ifndef _WIN32
#include <sys/resource.h>
#endif

#if USE_STXXL_LIBRARY
#include <stxxl/mng>
#endif

namespace osrm
{
namespace util
{

inline void DumpSTXXLStats()
{
#if USE_STXXL_LIBRARY
#if STXXL_VERSION_MAJOR > 1 || (STXXL_VERSION_MAJOR == 1 && STXXL_VERSION_MINOR >= 4)
    auto manager = stxxl::block_manager::get_instance();
    util::Log() << "STXXL: peak bytes used: " << manager->get_maximum_allocation();
    util::Log() << "STXXL: total disk allocated: " << manager->get_total_bytes();
#else
#warning STXXL 1.4+ recommended - STXXL memory summary will not be available
    util::Log() << "STXXL: memory summary not available, needs STXXL 1.4 or higher";
#endif
#endif
}

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
}
}

#endif
