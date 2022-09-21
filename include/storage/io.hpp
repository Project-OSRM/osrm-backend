#ifndef OSRM_STORAGE_IO_HPP_
#define OSRM_STORAGE_IO_HPP_

#include "osrm/error_codes.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/fingerprint.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/version.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/seek.hpp>
#include <boost/iostreams/stream.hpp>

#include <cerrno>
#include <cstring>
#include <tuple>
#include <type_traits>

namespace osrm
{
namespace storage
{
namespace io
{

class FileReader
{
  public:
    enum FingerprintFlag
    {
        VerifyFingerprint,
        HasNoFingerprint
    };

    FileReader(const std::string &filename, const FingerprintFlag flag)
        : FileReader(boost::filesystem::path(filename), flag)
    {
    }

    FileReader(const boost::filesystem::path &filepath_, const FingerprintFlag flag)
        : filepath(filepath_), fingerprint(flag)
    {
        input_stream.open(filepath, std::ios::binary);

        // Note: filepath.string() is wrapped in std::string() because it can
        // return char * on some platforms, which makes the + operator not work
        if (!input_stream)
            throw util::RuntimeError(
                filepath.string(), ErrorCode::FileOpenError, SOURCE_REF, std::strerror(errno));

        if (flag == VerifyFingerprint && !ReadAndCheckFingerprint())
        {
            throw util::RuntimeError(filepath.string(), ErrorCode::InvalidFingerprint, SOURCE_REF);
        }
    }

    std::size_t GetSize()
    {
        const boost::filesystem::path path(filepath);
        try
        {
            return std::size_t(boost::filesystem::file_size(path)) -
                   ((fingerprint == FingerprintFlag::VerifyFingerprint) ? sizeof(util::FingerPrint)
                                                                        : 0);
        }
        catch (const boost::filesystem::filesystem_error &ex)
        {
            std::cout << ex.what() << std::endl;
            throw;
        }
    }

    /* Read one line */
    template <typename T> void ReadLine(T *dest, const std::size_t count)
    {
        if (0 < count)
        {
            memset(dest, 0, count * sizeof(T));
            input_stream.getline(reinterpret_cast<char *>(dest), count * sizeof(T));
        }
    }

    /* Read count objects of type T into pointer dest */
    template <typename T> void ReadInto(T *dest, const std::size_t count)
    {
#if !defined(__GNUC__) || (__GNUC__ > 4)
        static_assert(!std::is_pointer<T>::value, "saving pointer types is not allowed");
        static_assert(std::is_trivially_copyable<T>::value,
                      "bytewise reading requires trivially copyable type");
#endif

        if (count == 0)
            return;

        const auto &result = input_stream.read(reinterpret_cast<char *>(dest), count * sizeof(T));
        const std::size_t bytes_read = input_stream.gcount();

        if (bytes_read != count * sizeof(T) && !result)
        {
            if (result.eof())
            {
                throw util::RuntimeError(
                    filepath.string(), ErrorCode::UnexpectedEndOfFile, SOURCE_REF);
            }
            throw util::RuntimeError(
                filepath.string(), ErrorCode::FileReadError, SOURCE_REF, std::strerror(errno));
        }
    }

    template <typename T, typename OutIter> void ReadStreaming(OutIter out, const std::size_t count)
    {
#if !defined(__GNUC__) || (__GNUC__ > 4)
        static_assert(!std::is_pointer<T>::value, "saving pointer types is not allowed");
        static_assert(std::is_trivially_copyable<T>::value,
                      "bytewise reading requires trivially copyable type");
#endif

        if (count == 0)
            return;

        T tmp;
        for (auto index : util::irange<std::size_t>(0, count))
        {
            (void)index;
            const auto &result = input_stream.read(reinterpret_cast<char *>(&tmp), sizeof(T));
            const std::size_t bytes_read = input_stream.gcount();

            if (bytes_read != sizeof(T) && !result)
            {
                if (result.eof())
                {
                    throw util::RuntimeError(
                        filepath.string(), ErrorCode::UnexpectedEndOfFile, SOURCE_REF);
                }
                throw util::RuntimeError(
                    filepath.string(), ErrorCode::FileReadError, SOURCE_REF, std::strerror(errno));
            }
            *out++ = tmp;
        }
    }

    template <typename T> void ReadInto(std::vector<T> &target)
    {
        ReadInto(target.data(), target.size());
    }

    template <typename T> void ReadInto(T &target) { ReadInto(&target, 1); }

    template <typename T> void Skip(const std::size_t element_count)
    {
        boost::iostreams::seek(input_stream, element_count * sizeof(T), BOOST_IOS::cur);
    }

    /*******************************************/

    std::uint64_t ReadElementCount64()
    {
        std::uint64_t count;
        ReadInto(count);
        return count;
    }

    template <typename T> std::size_t ReadVectorSize()
    {
        const auto count = ReadElementCount64();
        Skip<T>(count);
        return count;
    }

    bool ReadAndCheckFingerprint()
    {
        util::FingerPrint loaded_fingerprint;
        ReadInto(loaded_fingerprint);
        const auto expected_fingerprint = util::FingerPrint::GetValid();

        if (!loaded_fingerprint.IsValid())
        {
            throw util::RuntimeError(filepath.string(), ErrorCode::InvalidFingerprint, SOURCE_REF);
        }

        if (!expected_fingerprint.IsDataCompatible(loaded_fingerprint))
        {
            const std::string fileversion =
                std::to_string(loaded_fingerprint.GetMajorVersion()) + "." +
                std::to_string(loaded_fingerprint.GetMinorVersion()) + "." +
                std::to_string(loaded_fingerprint.GetPatchVersion());
            throw util::RuntimeError(std::string(filepath.string()) + " prepared with OSRM " +
                                         fileversion + " but this is " + OSRM_VERSION,
                                     ErrorCode::IncompatibleFileVersion,
                                     SOURCE_REF);
        }

        return true;
    }

  private:
    const boost::filesystem::path filepath;
    boost::filesystem::ifstream input_stream;
    FingerprintFlag fingerprint;
};

class FileWriter
{
  public:
    enum FingerprintFlag
    {
        GenerateFingerprint,
        HasNoFingerprint
    };

    FileWriter(const std::string &filename, const FingerprintFlag flag)
        : FileWriter(boost::filesystem::path(filename), flag)
    {
    }

    FileWriter(const boost::filesystem::path &filepath_, const FingerprintFlag flag)
        : filepath(filepath_), fingerprint(flag)
    {
        output_stream.open(filepath, std::ios::binary);
        if (!output_stream)
        {
            throw util::RuntimeError(
                filepath.string(), ErrorCode::FileOpenError, SOURCE_REF, std::strerror(errno));
        }

        if (flag == GenerateFingerprint)
        {
            WriteFingerprint();
        }
    }

    /* Write count objects of type T from pointer src to output stream */
    template <typename T> void WriteFrom(const T *src, const std::size_t count)
    {
#if !defined(__GNUC__) || (__GNUC__ > 4)
        static_assert(std::is_trivially_copyable<T>::value,
                      "bytewise writing requires trivially copyable type");
#endif

        if (count == 0)
            return;

        const auto &result =
            output_stream.write(reinterpret_cast<const char *>(src), count * sizeof(T));

        if (!result)
        {
            throw util::RuntimeError(
                filepath.string(), ErrorCode::FileWriteError, SOURCE_REF, std::strerror(errno));
        }
    }

    template <typename T> void WriteFrom(const std::vector<T> &src)
    {
        WriteFrom(src.data(), src.size());
    }

    template <typename T> void WriteFrom(const T &src) { WriteFrom(&src, 1); }

    void WriteElementCount64(const std::uint64_t count) { WriteFrom(count); }

    void WriteFingerprint()
    {
        const auto fingerprint = util::FingerPrint::GetValid();
        return WriteFrom(fingerprint);
    }

    template <typename T> void Skip(const std::size_t element_count)
    {
        boost::iostreams::seek(output_stream, element_count * sizeof(T), BOOST_IOS::cur);
    }

    void SkipToBeginning()
    {
        boost::iostreams::seek(output_stream, 0, std::ios::beg);

        // If we wrote a Fingerprint, skip over it
        if (fingerprint == FingerprintFlag::GenerateFingerprint)
            Skip<util::FingerPrint>(1);

        // Should probably return a functor for jumping back to the current pos.
    }

  private:
    const boost::filesystem::path filepath;
    boost::filesystem::ofstream output_stream;
    FingerprintFlag fingerprint;
};

class BufferReader
{
  public:
    BufferReader(const std::string &buffer) : BufferReader(buffer.data(), buffer.size()) {}

    BufferReader(const char *buffer, const std::size_t size)
        : input_stream(boost::iostreams::array_source(buffer, size))
    {
        if (!input_stream)
        {
            throw util::RuntimeError(
                "<buffer>", ErrorCode::FileOpenError, SOURCE_REF, std::strerror(errno));
        }
    }

    std::size_t GetPosition() { return input_stream.tellg(); }

    template <typename T> void ReadInto(T *dest, const std::size_t count)
    {
#if !defined(__GNUC__) || (__GNUC__ > 4)
        static_assert(!std::is_pointer<T>::value, "saving pointer types is not allowed");
        static_assert(std::is_trivially_copyable<T>::value,
                      "bytewise reading requires trivially copyable type");
#endif

        if (count == 0)
            return;

        const auto &result = input_stream.read(reinterpret_cast<char *>(dest), count * sizeof(T));
        const std::size_t bytes_read = input_stream.gcount();

        if (bytes_read != count * sizeof(T) && !result)
        {
            if (result.eof())
            {
                throw util::RuntimeError("<buffer>", ErrorCode::UnexpectedEndOfFile, SOURCE_REF);
            }
            throw util::RuntimeError(
                "<buffer>", ErrorCode::FileReadError, SOURCE_REF, std::strerror(errno));
        }
    }

    template <typename T> void ReadInto(T &tmp) { ReadInto(&tmp, 1); }

    std::uint64_t ReadElementCount64()
    {
        std::uint64_t count;
        ReadInto(count);
        return count;
    }

  private:
    boost::iostreams::stream<boost::iostreams::array_source> input_stream;
};

class BufferWriter
{
  public:
    BufferWriter() : output_stream(std::ios::binary)
    {
        if (!output_stream)
        {
            throw util::RuntimeError(
                "<buffer>", ErrorCode::FileOpenError, SOURCE_REF, std::strerror(errno));
        }
    }

    template <typename T> void WriteFrom(const T *src, const std::size_t count)
    {
#if !defined(__GNUC__) || (__GNUC__ > 4)
        static_assert(std::is_trivially_copyable<T>::value,
                      "bytewise writing requires trivially copyable type");
#endif

        if (count == 0)
            return;

        const auto &result =
            output_stream.write(reinterpret_cast<const char *>(src), count * sizeof(T));

        if (!result)
        {
            throw util::RuntimeError(
                "<buffer>", ErrorCode::FileWriteError, SOURCE_REF, std::strerror(errno));
        }
    }

    template <typename T> void WriteFrom(const T &tmp) { WriteFrom(&tmp, 1); }

    void WriteElementCount64(const std::uint64_t count) { WriteFrom(count); }

    std::string GetBuffer() const { return output_stream.str(); }

  private:
    std::ostringstream output_stream;
};
} // namespace io
} // namespace storage
} // namespace osrm

#endif
