#include "storage/shared_memory.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdlib>

BOOST_AUTO_TEST_SUITE(shared_memory)

using namespace osrm::storage;

BOOST_AUTO_TEST_CASE(get_lock_dir_uses_home_by_default)
{
    // Ensure SHM_LOCK_DIR is not set
    ::unsetenv("SHM_LOCK_DIR");

    const char *home = std::getenv("HOME");
    // Skip if HOME is not set (e.g. in containers)
    if (!home)
        return;

    auto lock_dir = getLockDir();
    auto expected = std::filesystem::path(home) / ".osrm";
    BOOST_CHECK_EQUAL(lock_dir, expected);
    BOOST_CHECK(std::filesystem::exists(lock_dir));
}

BOOST_AUTO_TEST_CASE(get_lock_dir_respects_shm_lock_dir_env)
{
    auto temp = std::filesystem::temp_directory_path() / "osrm-test-lock-dir";
    std::filesystem::create_directories(temp);

    ::setenv("SHM_LOCK_DIR", temp.c_str(), 1);
    auto lock_dir = getLockDir();
    BOOST_CHECK_EQUAL(lock_dir, temp);

    ::unsetenv("SHM_LOCK_DIR");
    std::filesystem::remove(temp);
}

BOOST_AUTO_TEST_CASE(get_lock_dir_shm_lock_dir_takes_priority_over_home)
{
    auto temp = std::filesystem::temp_directory_path() / "osrm-test-lock-dir-priority";
    std::filesystem::create_directories(temp);

    ::setenv("SHM_LOCK_DIR", temp.c_str(), 1);
    auto lock_dir = getLockDir();
    // Even if HOME is set, SHM_LOCK_DIR should win
    BOOST_CHECK_EQUAL(lock_dir, temp);

    ::unsetenv("SHM_LOCK_DIR");
    std::filesystem::remove(temp);
}

BOOST_AUTO_TEST_CASE(get_lock_dir_falls_back_to_tmp_without_home)
{
    ::unsetenv("SHM_LOCK_DIR");

    // Save and unset HOME
    const char *home = std::getenv("HOME");
    std::string saved_home;
    if (home)
        saved_home = home;
    ::unsetenv("HOME");

    auto lock_dir = getLockDir();
    BOOST_CHECK_EQUAL(lock_dir, std::filesystem::temp_directory_path());

    // Restore HOME
    if (!saved_home.empty())
        ::setenv("HOME", saved_home.c_str(), 1);
}

BOOST_AUTO_TEST_SUITE_END()
