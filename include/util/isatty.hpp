#ifndef ISATTY_HPP
#define ISATTY_HPP

// For isatty()
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#else
#ifdef WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#error Unknown platform - isatty implementation required
#endif // win32
#endif // unix

#include <cstdio>

namespace osrm
{
namespace util
{

// Returns true if stdout is a tty, false otherwise
//   Useful for when you want to do something different when
//   output is redirected to a logfile
inline bool IsStdoutATTY() { return isatty(fileno(stdout)); }

} // namespace util
} // namespace osrm

#endif
