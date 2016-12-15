#ifndef OSRM_STACKTRACE_HPP_
#define OSRM_STACKTRACE_HPP_

#include <cstdlib>
#include <iostream>
#include <system_error>

#ifdef OSRM_HAS_STACKTRACE
// From vendored third_party/stacktrace for now; will be included in Boost in the future
#include <boost/stacktrace.hpp>

#include <errno.h>  // for errno
#include <signal.h> // for sigaction, sigemptyset, siginfo_t
#include <string.h> // for strsignal
#endif

namespace osrm
{
namespace util
{

#ifdef OSRM_HAS_STACKTRACE
inline void InstallStacktraceHandler()
{
    struct ::sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    ::sigemptyset(&sa.sa_mask);

    sa.sa_sigaction = [](int, ::siginfo_t *si, void *) {
        boost::stacktrace::stacktrace bt;

        std::cerr << "Signal '" << ::strsignal(si->si_signo) << "' at address " << si->si_addr
                  << " from " << si->si_call_addr << ".\n";

        if (bt)
            std::cerr << "Backtrace:\n" << boost::stacktrace::stacktrace() << "\n";

        std::cerr << std::endl;
        std::exit(EXIT_FAILURE);
    };

    if (::sigaction(SIGSEGV, &sa, nullptr) == -1)
        throw std::system_error{errno, std::system_category()};

    if (::sigaction(SIGABRT, &sa, nullptr) == -1)
        throw std::system_error{errno, std::system_category()};

    if (::sigaction(SIGBUS, &sa, nullptr) == -1)
        throw std::system_error{errno, std::system_category()};
}

#else
inline void InstallStacktraceHandler() {}
#endif

} // namespace util
} // namespace osrm

#endif
