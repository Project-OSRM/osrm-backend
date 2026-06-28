#ifndef ISATTY_HPP
#define ISATTY_HPP

// For isatty()
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#elif defined(_WIN32) || defined(WIN32)
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#error Unknown platform - isatty implementation required
#endif

#include <cstdio>

namespace osrm::util
{

// Returns true if stdout is a tty, false otherwise
//   Useful for when you want to do something different when
//   output is redirected to a logfile
inline bool IsStdoutATTY() { return isatty(fileno(stdout)); }

} // namespace osrm::util

#endif
