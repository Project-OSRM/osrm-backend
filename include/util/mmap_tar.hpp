#ifndef OSRM_UTIL_MMAP_TAR_HPP
#define OSRM_UTIL_MMAP_TAR_HPP

#include "storage/tar.hpp"

#include "util/mmap_file.hpp"

#include <boost/iostreams/device/mapped_file.hpp>

#include <tuple>
#include <unordered_map>

namespace osrm
{
namespace util
{
using DataRange = std::pair<const char *, const char *>;
using DataMap = std::unordered_map<std::string, DataRange>;

inline DataMap mmapTarFile(const boost::filesystem::path &path,
                           boost::iostreams::mapped_file_source &region)
{
    DataMap map;

    storage::tar::FileReader reader{path, storage::tar::FileReader::VerifyFingerprint};

    std::vector<storage::tar::FileReader::FileEntry> entries;
    reader.List(std::back_inserter(entries));

    auto raw_file = mmapFile<char>(path, region);

    for (const auto &entry : entries)
    {
        auto begin = raw_file.data() + entry.offset;
        auto end = begin + entry.size;
        map[entry.name] = DataRange{begin, end};
    }

    return map;
}
}
}

#endif
