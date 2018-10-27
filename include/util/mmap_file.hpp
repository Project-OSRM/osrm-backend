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
template <typename T, typename MmapContainerT>
util::vector_view<T> mmapFile(const boost::filesystem::path &file, MmapContainerT &mmap_container)
{
    try
    {
        mmap_container.open(file);
        std::size_t num_objects = mmap_container.size() / sizeof(T);
        auto data_ptr = mmap_container.data();
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

template <typename T, typename MmapContainerT>
util::vector_view<T> mmapFile(const boost::filesystem::path &file,
                              MmapContainerT &mmap_container,
                              const std::size_t size)
{
    try
    {
        // Create a new file with the given size in bytes
        boost::iostreams::mapped_file_params params;
        params.path = file.string();
        params.flags = boost::iostreams::mapped_file::readwrite;
        params.new_file_size = size;
        mmap_container.open(params);

        std::size_t num_objects = size / sizeof(T);
        auto data_ptr = mmap_container.data();
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
                                    boost::iostreams::mapped_file_source &mmap_container)
{
    return detail::mmapFile<const T>(file, mmap_container);
}

template <typename T>
util::vector_view<T> mmapFile(const boost::filesystem::path &file,
                              boost::iostreams::mapped_file &mmap_container)
{
    return detail::mmapFile<T>(file, mmap_container);
}

template <typename T>
util::vector_view<T> mmapFile(const boost::filesystem::path &file,
                              boost::iostreams::mapped_file &mmap_container,
                              std::size_t size)
{
    return detail::mmapFile<T>(file, mmap_container, size);
}
}
}

#endif
