#ifndef OSRM_UTIL_MMAP_FILE_HPP
#define OSRM_UTIL_MMAP_FILE_HPP

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/vector_view.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

namespace osrm
{
namespace util
{

namespace detail
{
template <typename T, typename RegionT>
util::vector_view<T> mmapFile(const boost::filesystem::path &file, RegionT &region)
{
    try
    {
        region.open(file);
        std::size_t num_objects = region.size() / sizeof(T);
        auto data_ptr = region.data();
        BOOST_ASSERT(reinterpret_cast<uintptr_t>(data_ptr) % alignof(T) == 0);
        return util::vector_view<T>(reinterpret_cast<T *>(data_ptr), num_objects);
    }
    catch (const std::exception &exc)
    {
        throw exception(
            boost::str(boost::format("File %1% mapping failed: %2%") % file % exc.what()) +
            SOURCE_REF);
    }
}
}

template <typename T>
util::vector_view<const T> mmapFile(const boost::filesystem::path &file,
                                    boost::iostreams::mapped_file_source &region)
{
    return detail::mmapFile<const T>(file, region);
}

template <typename T>
util::vector_view<T> mmapFile(const boost::filesystem::path &file,
                              boost::iostreams::mapped_file &region)
{
    return detail::mmapFile<T>(file, region);
}
}
}

#endif
