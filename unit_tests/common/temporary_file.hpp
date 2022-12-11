#ifndef UNIT_TESTS_TEMPORARY_FILE_HPP
#define UNIT_TESTS_TEMPORARY_FILE_HPP

#include <chrono>
#include <filesystem>
#include <random>
#include <string>

struct TemporaryFile
{
    TemporaryFile() : path(std::filesystem::temp_directory_path() /= random_string(8)) {}
    TemporaryFile(const std::string &path) : path(path) {}

    ~TemporaryFile() { std::filesystem::remove(path); }

    std::filesystem::path path;

  private:
    std::string random_string(std::string::size_type length)
    {
        static auto &chrs = "0123456789"
                            "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 rg{seed1};
        std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

        std::string s;

        s.reserve(length);

        while (length--)
            s += chrs[pick(rg)];

        return s;
    }
};

#endif // UNIT_TESTS_TEMPORARY_FILE_HPP
