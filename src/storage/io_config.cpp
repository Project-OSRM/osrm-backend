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
    bool success = true;
    for (auto &fileName : required_input_files)
    {
        boost::system::error_code ec;
        if (!boost::filesystem::is_regular_file({base_path.string() + fileName.string()},ec))
        {
            util::Log(logWARNING) << "Missing/Broken File: " << base_path.string()
                                  << fileName.string();
            success = false;
        }
    }
    return success;
}
}
}
