#ifndef UNIT_TESTS_TEMPORARY_FILE_HPP
#define UNIT_TESTS_TEMPORARY_FILE_HPP

#include <boost/filesystem.hpp>

struct TemporaryFile
{
    TemporaryFile() : path(boost::filesystem::unique_path()) {}
    TemporaryFile(const std::string &path) : path(path) {}

    ~TemporaryFile() { boost::filesystem::remove(path); }

    boost::filesystem::path path;
};

#endif // UNIT_TESTS_TEMPORARY_FILE_HPP
