#include "storage/shared_memory.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <filesystem>
#include <string>

// RAII guard that saves and restores an environment variable
struct EnvGuard
{
    std::string name;
    std::string saved_value;
    bool was_set;

    EnvGuard(const std::string &name_) : name(name_)
    {
        const char *val = std::getenv(name.c_str());
        was_set = (val != nullptr);
        if (was_set)
            saved_value = val;
    }

    ~EnvGuard()
    {
        if (was_set)
        {
#ifdef _WIN32
            _putenv_s(name.c_str(), saved_value.c_str());
#else
            ::setenv(name.c_str(), saved_value.c_str(), 1);
#endif
        }
        else
        {
#ifdef _WIN32
            _putenv_s(name.c_str(), "");
#else
            ::unsetenv(name.c_str());
#endif
        }
    }

    static void set(const std::string &name, const std::string &value)
    {
#ifdef _WIN32
        _putenv_s(name.c_str(), value.c_str());
#else
        ::setenv(name.c_str(), value.c_str(), 1);
#endif
    }

    static void unset(const std::string &name)
    {
#ifdef _WIN32
        _putenv_s(name.c_str(), "");
#else
        ::unsetenv(name.c_str());
#endif
    }
};

BOOST_AUTO_TEST_SUITE(shared_memory)

using namespace osrm::storage;

BOOST_AUTO_TEST_CASE(get_lock_dir_respects_shm_lock_dir_env)
{
    EnvGuard shm_guard("SHM_LOCK_DIR");

    auto temp = std::filesystem::temp_directory_path() / "osrm-test-lock-dir";
    std::filesystem::create_directories(temp);

    EnvGuard::set("SHM_LOCK_DIR", temp.string());
    auto lock_dir = getLockDir();
    BOOST_CHECK_EQUAL(lock_dir, temp);

    std::filesystem::remove(temp);
}

BOOST_AUTO_TEST_CASE(get_lock_dir_shm_lock_dir_takes_priority)
{
    EnvGuard shm_guard("SHM_LOCK_DIR");
    EnvGuard home_guard("HOME");

    auto temp = std::filesystem::temp_directory_path() / "osrm-test-lock-dir-priority";
    std::filesystem::create_directories(temp);

    EnvGuard::set("SHM_LOCK_DIR", temp.string());
    auto lock_dir = getLockDir();
    // Even if HOME is set, SHM_LOCK_DIR should win
    BOOST_CHECK_EQUAL(lock_dir, temp);

    std::filesystem::remove(temp);
}

#ifdef __linux__
BOOST_AUTO_TEST_CASE(get_lock_dir_uses_dev_shm_on_linux)
{
    EnvGuard shm_guard("SHM_LOCK_DIR");
    EnvGuard home_guard("HOME");

    EnvGuard::unset("SHM_LOCK_DIR");

    auto lock_dir = getLockDir();
    // Should be under /dev/shm/osrm-<uid>/
    BOOST_CHECK(lock_dir.string().find("/dev/shm/osrm-") != std::string::npos);
    BOOST_CHECK(std::filesystem::exists(lock_dir));

    std::filesystem::remove(lock_dir);
}
#endif

BOOST_AUTO_TEST_CASE(get_lock_dir_uses_home_as_fallback)
{
    EnvGuard shm_guard("SHM_LOCK_DIR");
    EnvGuard home_guard("HOME");

    auto temp_home = std::filesystem::temp_directory_path() / "osrm-test-home";
    std::filesystem::create_directories(temp_home);

    EnvGuard::unset("SHM_LOCK_DIR");
    EnvGuard::set("HOME", temp_home.string());

    // On Linux this will prefer /dev/shm, so we only test this on non-Linux
    // or when /dev/shm is not available
#ifndef __linux__
    auto lock_dir = getLockDir();
    auto expected = temp_home / ".osrm";
    BOOST_CHECK_EQUAL(lock_dir, expected);
    BOOST_CHECK(std::filesystem::exists(lock_dir));
#endif

    std::filesystem::remove_all(temp_home);
}

BOOST_AUTO_TEST_SUITE_END()
