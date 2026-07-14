#ifndef OSRM_STORAGE_TAR_HPP
#define OSRM_STORAGE_TAR_HPP

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/fingerprint.hpp"
#include "util/integer_range.hpp"
#include "util/version.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace osrm::storage::tar
{
namespace detail
{
inline int fseek64(std::FILE *file, std::int64_t offset, int origin)
{
#ifdef _WIN32
    return _fseeki64(file, offset, origin);
#else
    return fseeko(file, offset, origin);
#endif
}

inline void checkArchiveError(struct archive *a,
                              int ret,
                              const std::filesystem::path &filepath,
                              const std::string &name)
{
    if (ret == ARCHIVE_OK || ret == ARCHIVE_WARN)
        return;

    const char *err = archive_error_string(a);
    std::string msg = filepath.string() + " : " + name;
    if (err)
        msg += " : " + std::string(err);

    int eno = archive_errno(a);
    if (eno == ENOENT || eno == 0)
        throw util::RuntimeError(msg, ErrorCode::FileIOError, SOURCE_REF);
    else
        throw util::RuntimeError(msg, ErrorCode::FileIOError, SOURCE_REF, std::strerror(eno));
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

    FileReader(const std::filesystem::path &path, FingerprintFlag flag) : path(path)
    {
        file = std::fopen(path.string().c_str(), "rb");
        if (!file)
        {
            throw util::RuntimeError(
                path.string(), ErrorCode::FileOpenError, SOURCE_REF, std::strerror(errno));
        }

        BuildIndex();

        if (flag == VerifyFingerprint)
        {
            ReadAndCheckFingerprint();
        }
    }

    ~FileReader()
    {
        if (file)
            std::fclose(file);
    }

    FileReader(const FileReader &) = delete;
    FileReader &operator=(const FileReader &) = delete;

    std::uint64_t ReadElementCount64(const std::string &name)
    {
        std::uint64_t size;
        ReadInto(name + ".meta", size);
        return size;
    }

    template <typename T> void ReadInto(const std::string &name, T &tmp)
    { ReadInto(name, &tmp, 1); }

    template <typename T, typename OutIter> void ReadStreaming(const std::string &name, OutIter out)
    {
        auto it = index.find(name);
        if (it == index.end())
        {
            throw util::RuntimeError(path.string() + " : " + name,
                                     ErrorCode::FileIOError,
                                     SOURCE_REF,
                                     "entry not found");
        }

        const auto &entry = it->second;
        auto number_of_elements = entry.size / sizeof(T);
        auto expected_size = sizeof(T) * number_of_elements;
        if (entry.size != expected_size)
        {
            throw util::RuntimeError(name + ": Datatype size does not match file size.",
                                     ErrorCode::UnexpectedEndOfFile,
                                     SOURCE_REF);
        }

        if (detail::fseek64(file, static_cast<std::int64_t>(entry.offset), SEEK_SET) != 0)
        {
            throw util::RuntimeError(path.string() + " : " + name,
                                     ErrorCode::FileIOError,
                                     SOURCE_REF,
                                     std::strerror(errno));
        }

        T tmp;
        for (auto idx : util::irange<std::size_t>(0, number_of_elements))
        {
            (void)idx;
            if (std::fread(&tmp, sizeof(T), 1, file) != 1)
            {
                throw util::RuntimeError(path.string() + " : " + name,
                                         ErrorCode::FileReadError,
                                         SOURCE_REF,
                                         std::strerror(errno));
            }
            *out++ = tmp;
        }
    }

    template <typename T>
    void ReadInto(const std::string &name, T *data, const std::size_t number_of_elements)
    {
        auto it = index.find(name);
        if (it == index.end())
        {
            throw util::RuntimeError(path.string() + " : " + name,
                                     ErrorCode::FileIOError,
                                     SOURCE_REF,
                                     "entry not found");
        }

        const auto &entry = it->second;
        auto expected_size = sizeof(T) * number_of_elements;
        if (entry.size != expected_size)
        {
            throw util::RuntimeError(name + ": Datatype size does not match file size.",
                                     ErrorCode::UnexpectedEndOfFile,
                                     SOURCE_REF);
        }

        if (detail::fseek64(file, static_cast<std::int64_t>(entry.offset), SEEK_SET) != 0)
        {
            throw util::RuntimeError(path.string() + " : " + name,
                                     ErrorCode::FileIOError,
                                     SOURCE_REF,
                                     std::strerror(errno));
        }

        if (std::fread(reinterpret_cast<char *>(data), 1, entry.size, file) != entry.size)
        {
            throw util::RuntimeError(path.string() + " : " + name,
                                     ErrorCode::FileReadError,
                                     SOURCE_REF,
                                     std::strerror(errno));
        }
    }

    struct FileEntry
    {
        std::string name;
        std::size_t size;
        std::size_t offset;
    };

    template <typename OutIter> void List(OutIter out)
    {
        for (const auto &entry : entries)
        {
            *out++ = entry;
        }
    }

  private:
    void BuildIndex()
    {
        struct archive *a = archive_read_new();
        archive_read_support_format_tar(a);
        archive_read_support_format_gnutar(a);

        int ret = archive_read_open_filename(a, path.string().c_str(), 10240);
        if (ret != ARCHIVE_OK)
        {
            const char *err = archive_error_string(a);
            std::string errmsg = err ? err : "unknown error";
            archive_read_free(a);
            throw util::RuntimeError(
                path.string(), ErrorCode::FileOpenError, SOURCE_REF, errmsg.c_str());
        }

        struct archive_entry *ae;

        // Collect header positions for all entries in order.
        // archive_read_header_position returns the position of the first header
        // block for an entry. For pax entries with extended attributes (e.g. entries
        // > 8GB), this points to the pax extension header, not the actual file header,
        // so the naive "header_position + 512" calculation is wrong for such entries.
        //
        // Instead, we compute data offsets from consecutive header positions:
        // each entry's padded data ends exactly where the next entry's header begins.
        //   data_offset[i] = header_position[i+1] - ceil_to_512(size[i])
        struct RawEntry
        {
            std::string name;
            std::size_t size;
            std::int64_t header_pos;
            bool is_regular;
        };
        std::vector<RawEntry> all_entries;

        while (archive_read_next_header(a, &ae) == ARCHIVE_OK)
        {
            std::int64_t pos = archive_read_header_position(a);
            bool is_reg = (archive_entry_filetype(ae) == AE_IFREG);
            std::string name;
            std::size_t size = 0;
            if (is_reg)
            {
                name = archive_entry_pathname(ae);
                size = static_cast<std::size_t>(archive_entry_size(ae));
            }
            all_entries.push_back({std::move(name), size, pos, is_reg});
            archive_read_data_skip(a);
        }

        // After ARCHIVE_EOF, header_position points to the end-of-archive marker
        auto end_of_archive_pos = archive_read_header_position(a);

        archive_read_free(a);

        for (std::size_t i = 0; i < all_entries.size(); ++i)
        {
            if (!all_entries[i].is_regular)
                continue;

            auto padded_size = ((all_entries[i].size + 511) / 512) * 512;
            std::int64_t next_pos =
                (i + 1 < all_entries.size()) ? all_entries[i + 1].header_pos : end_of_archive_pos;
            std::size_t data_offset = static_cast<std::size_t>(next_pos) - padded_size;

            index[all_entries[i].name] = IndexEntry{data_offset, all_entries[i].size};
            entries.push_back(FileEntry{all_entries[i].name, all_entries[i].size, data_offset});
        }
    }

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

    struct IndexEntry
    {
        std::size_t offset;
        std::size_t size;
    };

    std::filesystem::path path;
    std::FILE *file = nullptr;
    std::unordered_map<std::string, IndexEntry> index;
    std::vector<FileEntry> entries;
};

class FileWriter
{
  public:
    enum FingerprintFlag
    {
        GenerateFingerprint,
        HasNoFingerprint
    };

    FileWriter(const std::filesystem::path &path, FingerprintFlag flag) : path(path)
    {
        a = archive_write_new();
        int ret = archive_write_set_format_pax_restricted(a);
        detail::checkArchiveError(a, ret, path, "set format pax restricted");
        ret = archive_write_open_filename(a, path.string().c_str());
        if (ret != ARCHIVE_OK)
        {
            const char *err = archive_error_string(a);
            std::string errmsg = err ? err : "unknown error";
            archive_write_free(a);
            a = nullptr;
            throw util::RuntimeError(
                path.string(), ErrorCode::FileOpenError, SOURCE_REF, errmsg.c_str());
        }

        if (flag == GenerateFingerprint)
        {
            WriteFingerprint();
        }
    }

    ~FileWriter()
    {
        if (a)
        {
            archive_write_close(a);
            archive_write_free(a);
        }
    }

    FileWriter(const FileWriter &) = delete;
    FileWriter &operator=(const FileWriter &) = delete;

    void WriteElementCount64(const std::string &name, const std::uint64_t count)
    { WriteFrom(name + ".meta", count); }

    template <typename T> void WriteFrom(const std::string &name, const T &data)
    { WriteFrom(name, &data, 1); }

    template <typename T, typename Iter>
    void WriteStreaming(const std::string &name, Iter iter, const std::uint64_t number_of_elements)
    {
        auto number_of_bytes = number_of_elements * sizeof(T);

        struct archive_entry *ae = archive_entry_new();
        archive_entry_set_pathname(ae, name.c_str());
        archive_entry_set_size(ae, static_cast<la_int64_t>(number_of_bytes));
        archive_entry_set_filetype(ae, AE_IFREG);
        archive_entry_set_perm(ae, 0644);

        int ret = archive_write_header(a, ae);
        archive_entry_free(ae);
        detail::checkArchiveError(a, ret, path, name);

        for (auto idx : util::irange<std::size_t>(0, number_of_elements))
        {
            (void)idx;
            T tmp = *iter++;
            auto written = archive_write_data(a, &tmp, sizeof(T));
            if (written < 0 || static_cast<std::size_t>(written) != sizeof(T))
            {
                detail::checkArchiveError(a, ARCHIVE_FATAL, path, name);
            }
        }
    }

    template <typename T>
    void WriteFrom(const std::string &name, const T *data, const std::size_t number_of_elements)
    {
        auto number_of_bytes = number_of_elements * sizeof(T);

        struct archive_entry *ae = archive_entry_new();
        archive_entry_set_pathname(ae, name.c_str());
        archive_entry_set_size(ae, static_cast<la_int64_t>(number_of_bytes));
        archive_entry_set_filetype(ae, AE_IFREG);
        archive_entry_set_perm(ae, 0644);

        int ret = archive_write_header(a, ae);
        archive_entry_free(ae);
        detail::checkArchiveError(a, ret, path, name);

        const char *data_ptr = reinterpret_cast<const char *>(data);
        std::size_t remaining = number_of_bytes;
        while (remaining > 0)
        {
            const auto written = archive_write_data(a, data_ptr, remaining);
            if (written <= 0 || static_cast<std::size_t>(written) > remaining)
            {
                detail::checkArchiveError(a, ARCHIVE_FATAL, path, name);
            }

            data_ptr += written;
            remaining -= static_cast<std::size_t>(written);
        }
    }

  private:
    void WriteFingerprint()
    {
        const auto fingerprint = util::FingerPrint::GetValid();
        WriteFrom("osrm_fingerprint.meta", fingerprint);
    }

    std::filesystem::path path;
    struct archive *a = nullptr;
};
} // namespace osrm::storage::tar

#endif
