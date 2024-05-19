#ifndef UNIT_TESTS_TEMPORARY_FILE_HPP
#define UNIT_TESTS_TEMPORARY_FILE_HPP

#include <cstdlib>
#include <filesystem>
#include <random>
#include <string>

inline std::string random_string(std::string::size_type length)
{
    static auto &chrs = "0123456789"
                        "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(
        0, sizeof(chrs) - 2);

    std::string s;
    s.reserve(length);

    while (length--)
    {
        s += chrs[pick(rg)];
    }
    return s;
}

struct TemporaryFile
{
    TemporaryFile() : path(std::filesystem::temp_directory_path() / random_string(8)) {}
    TemporaryFile(const std::string &path) : path(path) {}

    ~TemporaryFile() { std::filesystem::remove(path); }

    std::filesystem::path path;
};

#endif // UNIT_TESTS_TEMPORARY_FILE_HPP
