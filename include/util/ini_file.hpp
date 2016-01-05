#ifndef INI_FILE_HPP
#define INI_FILE_HPP

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <algorithm>
#include <string>

namespace osrm
{
namespace util
{

namespace
{

// support old capitalized option names by down-casing them with a regex replace
std::string read_file_lower_content(const boost::filesystem::path &path)
{
    boost::filesystem::fstream config_stream(path);
    std::string ini_file_content((std::istreambuf_iterator<char>(config_stream)),
                                 std::istreambuf_iterator<char>());
    std::transform(std::begin(ini_file_content), std::end(ini_file_content),
                   std::begin(ini_file_content), ::tolower);
    return ini_file_content;
}
}
}
}

#endif // INI_FILE_HPP
