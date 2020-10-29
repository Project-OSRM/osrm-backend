#include "storage/io_config.hpp"

#include "util/log.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace osrm
{
namespace storage
{
bool IOConfig::IsValid() const
{
    namespace fs = boost::filesystem;

    bool success = true;
    for (auto &fileName : required_input_files)
    {
        if (!fs::is_regular_file(fs::status(base_path.string() + fileName.string())))
        {
            util::Log(logWARNING) << "Missing/Broken File: " << base_path.string()
                                  << fileName.string();
            success = false;
        }
    }
    return success;
}

std::vector<std::string> IOConfig::GetMissingFiles() const
{
    std::vector<std::string> missingFiles;
    for (auto &fileName : required_input_files)
    {
        if (!boost::filesystem::is_regular_file(boost::filesystem::path(base_path.string() + fileName.string())))
        {
            missingFiles.push_back(base_path.string() + fileName.string());
        }
    }
    return missingFiles;
}
} // namespace storage
} // namespace osrm
