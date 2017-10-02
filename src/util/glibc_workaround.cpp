#ifdef GLIBC_WORKAROUND
#include <stdexcept>

// https://github.com/bitcoin/bitcoin/pull/4042
// allows building against libstdc++-dev-4.9 while avoiding
// GLIBCXX_3.4.20 dep
// This is needed because libstdc++ itself uses this API - its not
// just an issue of your code using it, ughhh

// Note: only necessary on Linux
#ifdef __linux__
#define _ENABLE_GLIBC_WORKAROUND
#warning building with workaround
#else
#warning not building with workaround
#endif

#ifdef _ENABLE_GLIBC_WORKAROUND
namespace std
{

void __throw_out_of_range_fmt(const char *, ...) __attribute__((__noreturn__));
void __throw_out_of_range_fmt(const char *err, ...)
{
    // Safe and over-simplified version. Ignore the format and print it as-is.
    __throw_out_of_range(err);
}
}
#endif // _ENABLE_GLIBC_WORKAROUND

#endif // GLIBC_WORKAROUND