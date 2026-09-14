#ifndef OSRM_UTIL_AVAILABLE_THREADS_HPP
#define OSRM_UTIL_AVAILABLE_THREADS_HPP

#include <algorithm>
#include <cstdio>
#include <thread>

#ifdef __linux__
#include <cstdlib>
#include <cstring>
#endif

namespace osrm::util
{

#ifdef __linux__
namespace detail
{
// Returns the thread count implied by this process's cgroup CPU quota, or 0 if no quota applies
// (unlimited, or the cgroup files can't be read).
inline unsigned int GetCgroupThreadLimit()
{
    // cgroup v2: a single "cpu.max" file containing "<quota> <period>", or "max <period>" for
    // an unlimited quota.
    if (FILE *file = std::fopen("/sys/fs/cgroup/cpu.max", "r"))
    {
        char quota_str[32] = {};
        long long period = 0;
        const int matched = std::fscanf(file, "%31s %lld", quota_str, &period);
        std::fclose(file);
        if (matched == 2 && std::strcmp(quota_str, "max") != 0)
        {
            const long long quota = std::atoll(quota_str);
            if (quota > 0 && period > 0)
            {
                return static_cast<unsigned int>((quota + period - 1) / period);
            }
        }
        return 0;
    }

    // cgroup v1: separate cpu.cfs_quota_us / cpu.cfs_period_us files; a quota of -1 means
    // unlimited.
    FILE *quota_file = std::fopen("/sys/fs/cgroup/cpu/cpu.cfs_quota_us", "r");
    FILE *period_file = std::fopen("/sys/fs/cgroup/cpu/cpu.cfs_period_us", "r");
    unsigned int result = 0;
    if (quota_file && period_file)
    {
        long long quota = -1;
        long long period = 0;
        if (std::fscanf(quota_file, "%lld", &quota) == 1 &&
            std::fscanf(period_file, "%lld", &period) == 1 && quota > 0 && period > 0)
        {
            result = static_cast<unsigned int>((quota + period - 1) / period);
        }
    }
    if (quota_file)
        std::fclose(quota_file);
    if (period_file)
        std::fclose(period_file);
    return result;
}
} // namespace detail
#endif // __linux__

// Returns a sensible default thread count for this process: the container's cgroup CPU quota
// when one is set (Linux only), otherwise std::thread::hardware_concurrency(). Used as the
// --threads default across osrm-extract/partition/customize/contract/routed so a container
// capped below the host's full core count doesn't spin up more worker threads than its CPU
// share can actually run - under a CFS bandwidth limit the extra threads just get throttled,
// see https://github.com/Project-OSRM/osrm-backend/issues/6293.
inline unsigned int GetAvailableThreads()
{
    const auto hardware_threads = std::max<unsigned int>(1, std::thread::hardware_concurrency());
#ifdef __linux__
    const auto cgroup_threads = detail::GetCgroupThreadLimit();
    if (cgroup_threads > 0)
    {
        return std::min(cgroup_threads, hardware_threads);
    }
#endif
    return hardware_threads;
}

} // namespace osrm::util

#endif // OSRM_UTIL_AVAILABLE_THREADS_HPP
