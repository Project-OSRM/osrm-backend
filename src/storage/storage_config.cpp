#include "storage/storage_config.hpp"
#include "util/log.hpp"

#include <boost/filesystem/operations.hpp>

namespace osrm
{
namespace storage
{
namespace
{
bool CheckFileList(const std::vector<boost::filesystem::path> &files)
{
    bool success = true;
    for (auto &path : files)
    {
        if (!boost::filesystem::exists(path))
        {
            util::Log(logERROR) << "Missing File: " << path.string();
            success = false;
        }
    }
    return success;
}
}
}
}
