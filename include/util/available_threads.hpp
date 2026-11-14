#ifndef OSRM_UTIL_AVAILABLE_THREADS_HPP
#define OSRM_UTIL_AVAILABLE_THREADS_HPP

#include <algorithm>
#include <cstdio>
#include <thread>

#ifdef __linux__
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#endif

namespace osrm::util
{

#ifdef __linux__
namespace detail
{

// Returns the thread count implied by a cgroup v2 "cpu.max" file, or 0 if the quota is
// unlimited ("max"), unset, or the file can't be read.
inline unsigned int ReadCgroupV2Quota(const std::string &cpu_max_path)
{
    FILE *file = std::fopen(cpu_max_path.c_str(), "r");
    if (!file)
    {
        return 0;
    }
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

// Returns the thread count implied by a cgroup v1 cpu.cfs_quota_us/cpu.cfs_period_us pair in
// `cpu_dir`, or 0 if the quota is unlimited (-1), unset, or the files can't be read.
inline unsigned int ReadCgroupV1Quota(const std::string &cpu_dir)
{
    FILE *quota_file = std::fopen((cpu_dir + "/cpu.cfs_quota_us").c_str(), "r");
    FILE *period_file = std::fopen((cpu_dir + "/cpu.cfs_period_us").c_str(), "r");
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
    {
        std::fclose(quota_file);
    }
    if (period_file)
    {
        std::fclose(period_file);
    }
    return result;
}

// Returns this process's cgroup path, relative to the controller's mount root, by parsing
// `proc_cgroup_path` (normally /proc/self/cgroup). For cgroup v2 pass an empty `controller`;
// for v1 pass the controller name to look up (e.g. "cpu"). Returns an empty string - the
// hierarchy root - when the file can't be read or has no matching line, which is also the
// correct answer when the process's cgroup namespace is already scoped to its own cgroup (as
// in most container runtimes), since the mount root then *is* that cgroup.
inline std::string
GetOwnCgroupPath(bool is_v2, const std::string &controller, const std::string &proc_cgroup_path)
{
    std::ifstream file(proc_cgroup_path);
    if (!file)
    {
        return {};
    }
    std::string line;
    while (std::getline(file, line))
    {
        const auto first_colon = line.find(':');
        if (first_colon == std::string::npos)
        {
            continue;
        }
        const auto second_colon = line.find(':', first_colon + 1);
        if (second_colon == std::string::npos)
        {
            continue;
        }
        const std::string hierarchy = line.substr(first_colon + 1, second_colon - first_colon - 1);
        const std::string path = line.substr(second_colon + 1);
        if (is_v2)
        {
            // cgroup v2 unified hierarchy entries look like "0::/path"
            if (hierarchy.empty())
            {
                return path;
            }
        }
        else
        {
            // cgroup v1 entries look like "4:cpu,cpuacct:/path"
            std::stringstream controllers(hierarchy);
            std::string name;
            while (std::getline(controllers, name, ','))
            {
                if (name == controller)
                {
                    return path;
                }
            }
        }
    }
    return {};
}

// Walks a cgroup path from the process's own cgroup up to the controller's mount root, applying
// `read_quota` (a `std::string directory -> unsigned int` callable) at every level and returning
// the smallest non-zero result, or 0 if none of them set a quota. A parent cgroup's quota bounds
// its children without being reflected in a child's own quota file, so the whole chain has to be
// checked: https://docs.kernel.org/admin-guide/cgroup-v2.html
template <typename ReadQuota>
inline unsigned int
WalkCgroupHierarchy(const std::string &mount_root, std::string path, ReadQuota read_quota)
{
    unsigned int limit = 0;
    while (true)
    {
        const unsigned int quota = read_quota(mount_root + path);
        if (quota > 0 && (limit == 0 || quota < limit))
        {
            limit = quota;
        }
        if (path.empty())
        {
            break;
        }
        const auto pos = path.find_last_of('/');
        path = (pos == std::string::npos) ? std::string() : path.substr(0, pos);
    }
    return limit;
}

// Returns the thread count implied by this process's cgroup CPU quota (checking its own cgroup
// and every ancestor up to the mount root), or 0 if no quota applies anywhere in the chain.
// `cgroup_root` and `proc_cgroup_path` are overridable for testing; production code should use
// the defaults.
inline unsigned int GetCgroupThreadLimit(const std::string &cgroup_root = "/sys/fs/cgroup",
                                         const std::string &proc_cgroup_path = "/proc/self/cgroup")
{
    // cgroup v2: a single "cpu.max" file per cgroup, containing "<quota> <period>", or
    // "max <period>" for an unlimited quota.
    const std::string v2_path = GetOwnCgroupPath(true, "", proc_cgroup_path);
    const unsigned int v2_limit = WalkCgroupHierarchy(
        cgroup_root,
        v2_path,
        [](const std::string &dir) { return ReadCgroupV2Quota(dir + "/cpu.max"); });
    if (v2_limit > 0)
    {
        return v2_limit;
    }

    // cgroup v1: separate cpu.cfs_quota_us / cpu.cfs_period_us files per cgroup; a quota of -1
    // means unlimited.
    const std::string v1_path = GetOwnCgroupPath(false, "cpu", proc_cgroup_path);
    return WalkCgroupHierarchy(cgroup_root + "/cpu",
                               v1_path,
                               [](const std::string &dir) { return ReadCgroupV1Quota(dir); });
}
} // namespace detail
#endif // __linux__

// Returns a sensible default thread count for this process: the container's cgroup CPU quota
// when one is set (Linux only), otherwise std::thread::hardware_concurrency(). Used as the
// --threads default across osrm-extract/partition/customize/contract/routed so a container
// capped below the host's full core count doesn't spin up more worker threads than its CPU
// share can actually run - under a CFS bandwidth limit the extra threads just get throttled,
// see https://github.com/Project-OSRM/osrm-backend/issues/6293.
//
// This is only a default: it must not be used to clamp a thread count the user explicitly
// requested (see Server::CreateServer and generateServerProgramOptions), since an explicit
// --threads should be able to exceed a container's quota if the operator wants threads to
// contend for the available CPU share rather than be limited to it.
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
