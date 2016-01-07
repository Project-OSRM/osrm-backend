#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <boost/filesystem/path.hpp>

#include <unordered_map>
#include <string>

namespace osrm
{
namespace storage
{
using DataPaths = std::unordered_map<std::string, boost::filesystem::path>;
class Storage
{
public:
    Storage(const DataPaths& data_paths);
    int Run();
private:
    DataPaths paths;
};
}
}

#endif
