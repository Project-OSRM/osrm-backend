#ifndef OSRM_IO_CONFIG_HPP
#define OSRM_IO_CONFIG_HPP

#include "util/exception.hpp"
#include "util/log.hpp"

#include <array>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <string>

namespace osrm
{
namespace storage
{
struct IOConfig
{
    IOConfig(std::vector<boost::filesystem::path> _required_input_files,
             std::vector<boost::filesystem::path> _optional_input_files,
             std::vector<boost::filesystem::path> _output_files)
        : required_input_files(_required_input_files), optional_input_files(_optional_input_files),
          output_files(_output_files)
    {
    }

    // Infer the base path from the path of the .osrm file
    void UseDefaultOutputNames(const boost::filesystem::path &base)
    {
        // potentially strip off the .osrm (or other) extensions for
        // determining the base path=
        std::string path = base.string();

        std::array<std::string, 6> known_extensions{
            {".osm.bz2", ".osm.pbf", ".osm.xml", ".pbf", ".osm", ".osrm"}};
        for (auto ext : known_extensions)
        {
            const auto pos = path.find(ext);
            if (pos != std::string::npos)
            {
                path.replace(pos, ext.size(), "");
                break;
            }
        }

        base_path = {path};
    }

    bool IsValid() const
    {
        bool success = true;
        for (auto &fileName : required_input_files)
        {
            if (!boost::filesystem::is_regular_file({base_path.string() + fileName.string()}))
            {
                util::Log(logWARNING) << "Missing/Broken File: " << base_path.string()
                                      << fileName.string();
                success = false;
            }
        }
        return success;
    }

    boost::filesystem::path GetPath(const std::string &fileName) const
    {
        if (!IsConfigured(fileName, required_input_files) &&
            !IsConfigured(fileName, optional_input_files) && !IsConfigured(fileName, output_files))
        {
            util::Log(logERROR) << "Tried to access file which is not configured: " << fileName;
            throw util::exception("Tried to access file which is not configured: " + fileName);
        }

        return {base_path.string() + fileName};
    }

    boost::filesystem::path base_path;

  private:
    static bool IsConfigured(const std::string &fileName,
                             const std::vector<boost::filesystem::path> &paths)
    {
        for (auto &path : paths)
        {
            if (boost::algorithm::ends_with(path.string(), fileName))
            {
                return true;
            }
        }

        return false;
    }

    std::vector<boost::filesystem::path> required_input_files;
    std::vector<boost::filesystem::path> optional_input_files;
    std::vector<boost::filesystem::path> output_files;
};
}
}

#endif
