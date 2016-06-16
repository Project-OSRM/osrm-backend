#ifndef OSRM_COMPAT_HPP
#define OSRM_COMPAT_HPP

#if defined(__ARM_ARCH_7A__)

#include <string>
#include <sstream>

namespace std
{
    // See discussion https://answers.launchpad.net/gcc-arm-embedded/+question/238327
    template <typename T> std::string to_string(const T& n)
    {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }

    // These might be fixed with a more recent gcc. See discussion on w.r.t. to ChromeOS gcc:
    // https://code.google.com/p/android/issues/detail?id=54418
    inline double round(double d)
    {
        return ::round(d);
    }

    inline double hypot(double x, double y)
    {
        return ::hypot(x, y);
    }

    inline long long llabs(long long n)
    {
        return ::llabs(n);
    }
}

#endif

#endif // OSRM_COMPAT_HPP
