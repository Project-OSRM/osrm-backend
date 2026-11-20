#include "util/available_threads.hpp"

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#ifdef __linux__

BOOST_AUTO_TEST_SUITE(available_threads)

using namespace osrm::util::detail;
namespace fs = std::filesystem;

namespace
{
struct TempCgroupTree
{
    fs::path root;

    TempCgroupTree()
        : root(fs::temp_directory_path() /
               fs::path("osrm-available-threads-test-" +
                        std::to_string(reinterpret_cast<std::uintptr_t>(this))))
    { fs::remove_all(root); }

    ~TempCgroupTree() { fs::remove_all(root); }

    void write(const fs::path &relative, const std::string &content) const
    {
        const auto path = root / relative;
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        file << content;
    }

    std::string path() const { return root.string(); }
};
} // namespace

BOOST_AUTO_TEST_CASE(v2_quota_at_mount_root)
{
    TempCgroupTree tree;
    tree.write("proc_cgroup", "0::/\n");
    tree.write("cpu.max", "200000 100000\n"); // 2 CPUs

    BOOST_CHECK_EQUAL(GetCgroupThreadLimit(tree.path(), (tree.root / "proc_cgroup").string()), 2u);
}

BOOST_AUTO_TEST_CASE(v2_unlimited_returns_zero)
{
    TempCgroupTree tree;
    tree.write("proc_cgroup", "0::/\n");
    tree.write("cpu.max", "max 100000\n");

    BOOST_CHECK_EQUAL(GetCgroupThreadLimit(tree.path(), (tree.root / "proc_cgroup").string()), 0u);
}

// A process's own cgroup can be unlimited while a parent further up the hierarchy still caps it;
// the child's cpu.max alone would miss that: https://docs.kernel.org/admin-guide/cgroup-v2.html
BOOST_AUTO_TEST_CASE(v2_nested_ancestor_quota_is_found)
{
    TempCgroupTree tree;
    tree.write("proc_cgroup", "0::/a/b/c\n");
    tree.write("cpu.max", "max 100000\n");
    tree.write("a/cpu.max", "400000 100000\n"); // 4 CPUs, set on a non-leaf ancestor
    tree.write("a/b/cpu.max", "max 100000\n");
    tree.write("a/b/c/cpu.max", "max 100000\n"); // the process's own cgroup: unlimited

    BOOST_CHECK_EQUAL(GetCgroupThreadLimit(tree.path(), (tree.root / "proc_cgroup").string()), 4u);
}

BOOST_AUTO_TEST_CASE(v2_tighter_parent_quota_wins_over_looser_child)
{
    TempCgroupTree tree;
    tree.write("proc_cgroup", "0::/a/b\n");
    tree.write("cpu.max", "max 100000\n");
    tree.write("a/cpu.max", "200000 100000\n");   // parent: 2 CPUs
    tree.write("a/b/cpu.max", "800000 100000\n"); // child: 8 CPUs (looser, doesn't apply)

    BOOST_CHECK_EQUAL(GetCgroupThreadLimit(tree.path(), (tree.root / "proc_cgroup").string()), 2u);
}

BOOST_AUTO_TEST_CASE(v2_tighter_child_quota_wins_over_looser_parent)
{
    TempCgroupTree tree;
    tree.write("proc_cgroup", "0::/a/b\n");
    tree.write("cpu.max", "max 100000\n");
    tree.write("a/cpu.max", "800000 100000\n");   // parent: 8 CPUs (looser)
    tree.write("a/b/cpu.max", "200000 100000\n"); // child: 2 CPUs

    BOOST_CHECK_EQUAL(GetCgroupThreadLimit(tree.path(), (tree.root / "proc_cgroup").string()), 2u);
}

BOOST_AUTO_TEST_CASE(v1_nested_parent_quota_is_found)
{
    TempCgroupTree tree;
    tree.write("proc_cgroup", "4:cpu,cpuacct:/a/b\n");
    tree.write("cpu/cpu.cfs_quota_us", "-1\n");
    tree.write("cpu/cpu.cfs_period_us", "100000\n");
    tree.write("cpu/a/cpu.cfs_quota_us", "300000\n"); // parent: 3 CPUs
    tree.write("cpu/a/cpu.cfs_period_us", "100000\n");
    tree.write("cpu/a/b/cpu.cfs_quota_us", "-1\n"); // the process's own cgroup: unlimited
    tree.write("cpu/a/b/cpu.cfs_period_us", "100000\n");

    BOOST_CHECK_EQUAL(GetCgroupThreadLimit(tree.path(), (tree.root / "proc_cgroup").string()), 3u);
}

// When the process's cgroup namespace is already scoped to its own cgroup (the common container
// case), or /proc/self/cgroup simply can't be read, the mount root itself must still be checked.
BOOST_AUTO_TEST_CASE(missing_proc_cgroup_falls_back_to_mount_root)
{
    TempCgroupTree tree;
    tree.write("cpu.max", "100000 100000\n"); // 1 CPU

    BOOST_CHECK_EQUAL(GetCgroupThreadLimit(tree.path(), (tree.root / "no-such-file").string()), 1u);
}

BOOST_AUTO_TEST_SUITE_END()

#endif // __linux__
