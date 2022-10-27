#ifndef OSRM_STORAGE_TAR_HPP
#define OSRM_STORAGE_TAR_HPP

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/fingerprint.hpp"
#include "util/integer_range.hpp"
#include "util/version.hpp"

#include <boost/filesystem/path.hpp>

extern "C"
{
#include "microtar.h"
}

namespace osrm
{
namespace storage
{
namespace tar
{
namespace detail
{
inline void
checkMTarError(int error_code, const boost::filesystem::path &filepath, const std::string &name)
{
    switch (error_code)
    {
    case MTAR_ESUCCESS:
        return;
    case MTAR_EFAILURE:
        throw util::RuntimeError(
            filepath.string() + " : " + name, ErrorCode::FileIOError, SOURCE_REF);
    case MTAR_EOPENFAIL:
        throw util::RuntimeError(filepath.string() + " : " + name,
                                 ErrorCode::FileOpenError,
                                 SOURCE_REF,
                                 std::strerror(errno));
    case MTAR_EREADFAIL:
        throw util::RuntimeError(filepath.string() + " : " + name,
                                 ErrorCode::FileReadError,
                                 SOURCE_REF,
                                 std::strerror(errno));
    case MTAR_EWRITEFAIL:
        throw util::RuntimeError(filepath.string() + " : " + name,
                                 ErrorCode::FileWriteError,
                                 SOURCE_REF,
                                 std::strerror(errno));
    case MTAR_ESEEKFAIL:
        throw util::RuntimeError(filepath.string() + " : " + name,
                                 ErrorCode::FileIOError,
                                 SOURCE_REF,
                                 std::strerror(errno));
    case MTAR_EBADCHKSUM:
        throw util::RuntimeError(filepath.string() + " : " + name,
                                 ErrorCode::FileIOError,
                                 SOURCE_REF,
                                 std::strerror(errno));
    case MTAR_ENULLRECORD:
        throw util::RuntimeError(filepath.string() + " : " + name,
                                 ErrorCode::UnexpectedEndOfFile,
                                 SOURCE_REF,
                                 std::strerror(errno));
    case MTAR_ENOTFOUND:
        throw util::RuntimeError(filepath.string() + " : " + name,
                                 ErrorCode::FileIOError,
                                 SOURCE_REF,
                                 std::strerror(errno));
    default:
        throw util::exception(filepath.string() + " : " + name + ":" + mtar_strerror(error_code));
    }
}
} // namespace detail

class FileReader
{
  public:
    enum FingerprintFlag
    {
        VerifyFingerprint,
        HasNoFingerprint
    };

    FileReader(const boost::filesystem::path &path, FingerprintFlag flag) : path(path)
    {
        auto ret = mtar_open(&handle, path.string().c_str(), "r");
        detail::checkMTarError(ret, path, "");

        if (flag == VerifyFingerprint)
        {
            ReadAndCheckFingerprint();
        }
    }

    ~FileReader() { mtar_close(&handle); }

    std::uint64_t ReadElementCount64(const std::string &name)
    {
        std::uint64_t size;
        ReadInto(name + ".meta", size);
        return size;
    }

    template <typename T> void ReadInto(const std::string &name, T &tmp)
    {
        ReadInto(name, &tmp, 1);
    }

    template <typename T, typename OutIter> void ReadStreaming(const std::string &name, OutIter out)
    {
        mtar_header_t header;
        auto ret = mtar_find(&handle, name.c_str(), &header);
        detail::checkMTarError(ret, path, name);

        auto number_of_elements = header.size / sizeof(T);
        auto expected_size = sizeof(T) * number_of_elements;
        if (header.size != expected_size)
        {
            throw util::RuntimeError(name + ": Datatype size does not match file size.",
                                     ErrorCode::UnexpectedEndOfFile,
                                     SOURCE_REF);
        }

        T tmp;
        for (auto index : util::irange<std::size_t>(0, number_of_elements))
        {
            (void)index;
            ret = mtar_read_data(&handle, reinterpret_cast<char *>(&tmp), sizeof(T));
            detail::checkMTarError(ret, path, name);

            *out++ = tmp;
        }
    }

    template <typename T>
    void ReadInto(const std::string &name, T *data, const std::size_t number_of_elements)
    {
        mtar_header_t header;
        auto ret = mtar_find(&handle, name.c_str(), &header);
        detail::checkMTarError(ret, path, name);

        auto expected_size = sizeof(T) * number_of_elements;
        if (header.size != expected_size)
        {
            throw util::RuntimeError(name + ": Datatype size does not match file size.",
                                     ErrorCode::UnexpectedEndOfFile,
                                     SOURCE_REF);
        }

        ret = mtar_read_data(&handle, reinterpret_cast<char *>(data), header.size);
        detail::checkMTarError(ret, path, name);
    }

    struct FileEntry
    {
        std::string name;
        std::size_t size;
        std::size_t offset;
    };

    template <typename OutIter> void List(OutIter out)
    {
        mtar_header_t header;
        while (mtar_read_header(&handle, &header) != MTAR_ENULLRECORD)
        {
            if (header.type == MTAR_TREG)
            {
                int ret = mtar_read_data(&handle, nullptr, 0);
                detail::checkMTarError(ret, path, header.name);

                auto offset = handle.pos;
                // seek back to the header
                handle.remaining_data = 0;
                ret = mtar_seek(&handle, handle.last_header);
                detail::checkMTarError(ret, path, header.name);

                *out++ = FileEntry{header.name, header.size, offset};
            }
            mtar_next(&handle);
        }
    }

  private:
    bool ReadAndCheckFingerprint()
    {
        util::FingerPrint loaded_fingerprint;
        ReadInto("osrm_fingerprint.meta", loaded_fingerprint);
        const auto expected_fingerprint = util::FingerPrint::GetValid();

        if (!loaded_fingerprint.IsValid())
        {
            throw util::RuntimeError(path.string(), ErrorCode::InvalidFingerprint, SOURCE_REF);
        }

        if (!expected_fingerprint.IsDataCompatible(loaded_fingerprint))
        {
            const std::string fileversion =
                std::to_string(loaded_fingerprint.GetMajorVersion()) + "." +
                std::to_string(loaded_fingerprint.GetMinorVersion()) + "." +
                std::to_string(loaded_fingerprint.GetPatchVersion());
            throw util::RuntimeError(std::string(path.string()) + " prepared with OSRM " +
                                         fileversion + " but this is " + OSRM_VERSION,
                                     ErrorCode::IncompatibleFileVersion,
                                     SOURCE_REF);
        }

        return true;
    }

    boost::filesystem::path path;
    mtar_t handle;
};

class FileWriter
{
  public:
    enum FingerprintFlag
    {
        GenerateFingerprint,
        HasNoFingerprint
    };

    FileWriter(const boost::filesystem::path &path, FingerprintFlag flag) : path(path)
    {
        auto ret = mtar_open(&handle, path.string().c_str(), "w");
        detail::checkMTarError(ret, path, "");

        if (flag == GenerateFingerprint)
        {
            WriteFingerprint();
        }
    }

    ~FileWriter()
    {
        mtar_finalize(&handle);
        mtar_close(&handle);
    }

    void WriteElementCount64(const std::string &name, const std::uint64_t count)
    {
        WriteFrom(name + ".meta", count);
    }

    template <typename T> void WriteFrom(const std::string &name, const T &data)
    {
        WriteFrom(name, &data, 1);
    }

    template <typename T, typename Iter>
    void WriteStreaming(const std::string &name, Iter iter, const std::uint64_t number_of_elements)
    {
        auto number_of_bytes = number_of_elements * sizeof(T);

        auto ret = mtar_write_file_header(&handle, name.c_str(), number_of_bytes);
        detail::checkMTarError(ret, path, name);

        for (auto index : util::irange<std::size_t>(0, number_of_elements))
        {
            (void)index;
            T tmp = *iter++;
            ret = mtar_write_data(&handle, &tmp, sizeof(T));
            detail::checkMTarError(ret, path, name);
        }
    }

    // Continue writing an existing file, overwrites all data after the file!
    template <typename T>
    void ContinueFrom(const std::string &name, const T *data, const std::size_t number_of_elements)
    {
        auto number_of_bytes = number_of_elements * sizeof(T);

        mtar_header_t header;
        auto ret = mtar_find(&handle, name.c_str(), &header);
        detail::checkMTarError(ret, path, name);

        // update header to reflect increased tar size
        auto old_size = header.size;
        header.size += number_of_bytes;
        ret = mtar_write_header(&handle, &header);
        detail::checkMTarError(ret, path, name);

        // now seek to the end of the old record
        handle.remaining_data = number_of_bytes;
        ret = mtar_seek(&handle, handle.pos + old_size);
        detail::checkMTarError(ret, path, name);

        ret = mtar_write_data(&handle, data, number_of_bytes);
        detail::checkMTarError(ret, path, name);
    }

    template <typename T>
    void WriteFrom(const std::string &name, const T *data, const std::size_t number_of_elements)
    {
        auto number_of_bytes = number_of_elements * sizeof(T);

        auto ret = mtar_write_file_header(&handle, name.c_str(), number_of_bytes);
        detail::checkMTarError(ret, path, name);

        ret = mtar_write_data(&handle, reinterpret_cast<const char *>(data), number_of_bytes);
        detail::checkMTarError(ret, path, name);
    }

  private:
    void WriteFingerprint()
    {
        const auto fingerprint = util::FingerPrint::GetValid();
        WriteFrom("osrm_fingerprint.meta", fingerprint);
    }

    boost::filesystem::path path;
    mtar_t handle;
};
} // namespace tar
} // namespace storage
} // namespace osrm

#endif
