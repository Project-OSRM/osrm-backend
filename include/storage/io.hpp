#ifndef OSRM_STORAGE_IO_HPP_
#define OSRM_STORAGE_IO_HPP_

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/fingerprint.hpp"
#include "util/log.hpp"
#include "util/version.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/seek.hpp>

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
  private:
    const boost::filesystem::path filepath;
    boost::filesystem::ifstream input_stream;

  public:
    class LineWrapper : public std::string
    {
        friend std::istream &operator>>(std::istream &is, LineWrapper &line)
        {
            return std::getline(is, line);
        }
    };
    auto GetLineIteratorBegin() { return std::istream_iterator<LineWrapper>(input_stream); }
    auto GetLineIteratorEnd() { return std::istream_iterator<LineWrapper>(); }

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
        : filepath(filepath_)
    {
        input_stream.open(filepath, std::ios::binary);
        if (!input_stream)
            throw util::exception("Error opening " + filepath.string());

        if (flag == VerifyFingerprint && !ReadAndCheckFingerprint())
        {
            throw util::exception("Fingerprint mismatch in " + filepath_.string() + SOURCE_REF);
        }
    }

    std::size_t GetSize()
    {
        const boost::filesystem::ifstream::pos_type positon = input_stream.tellg();
        input_stream.seekg(0, std::ios::end);
        const boost::filesystem::ifstream::pos_type file_size = input_stream.tellg();

        if (file_size == boost::filesystem::ifstream::pos_type(-1))
        {
            throw util::exception("File size for " + filepath.string() + " failed " + SOURCE_REF);
        }

        // restore the current position
        input_stream.seekg(positon, std::ios::beg);
        return file_size;
    }

    /* Read count objects of type T into pointer dest */
    template <typename T> void ReadInto(T *dest, const std::size_t count)
    {
#if not defined __GNUC__ or __GNUC__ > 4
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
                throw util::exception("Error reading from " + filepath.string() +
                                      ": Unexpected end of file " + SOURCE_REF);
            }
            throw util::exception("Error reading from " + filepath.string() + " " + SOURCE_REF);
        }
    }

    template <typename T> void ReadInto(std::vector<T> &target)
    {
        ReadInto(target.data(), target.size());
    }

    template <typename T> void ReadInto(T &target) { ReadInto(&target, 1); }

    template <typename T> T ReadOne()
    {
        T tmp;
        ReadInto(tmp);
        return tmp;
    }

    template <typename T> void Skip(const std::size_t element_count)
    {
        boost::iostreams::seek(input_stream, element_count * sizeof(T), BOOST_IOS::cur);
    }

    /*******************************************/

    std::uint32_t ReadElementCount32() { return ReadOne<std::uint32_t>(); }
    std::uint64_t ReadElementCount64() { return ReadOne<std::uint64_t>(); }

    template <typename T> void DeserializeVector(std::vector<T> &data)
    {
        const auto count = ReadElementCount64();
        data.resize(count);
        ReadInto(data.data(), count);
    }

    template <typename T> std::size_t GetVectorMemorySize()
    {
        const auto count = ReadElementCount64();
        Skip<T>(count);
        return sizeof(count) + sizeof(T) * count;
    }

    template <typename T> void *DeserializeVector(void *begin, const void *end)
    {
        auto count = ReadElementCount64();
        auto required = reinterpret_cast<char *>(begin) + sizeof(count) + sizeof(T) * count;
        if (required > end)
            throw util::exception("Not enough memory ");

        *reinterpret_cast<decltype(count) *>(begin) = count;
        ReadInto(reinterpret_cast<T *>(reinterpret_cast<char *>(begin) + sizeof(decltype(count))),
                 count);
        return required;
    }

    bool ReadAndCheckFingerprint()
    {
        auto loaded_fingerprint = ReadOne<util::FingerPrint>();
        const auto expected_fingerprint = util::FingerPrint::GetValid();

        if (!loaded_fingerprint.IsValid())
        {
            util::Log(logERROR) << "Fingerprint magic number or checksum is invalid in "
                                << filepath.string();
            return false;
        }

        if (!expected_fingerprint.IsDataCompatible(loaded_fingerprint))
        {
            util::Log(logERROR) << filepath.string()
                                << " is not compatible with this version of OSRM";

            util::Log(logERROR) << "It was prepared with OSRM "
                                << loaded_fingerprint.GetMajorVersion() << "."
                                << loaded_fingerprint.GetMinorVersion() << "."
                                << loaded_fingerprint.GetPatchVersion() << " but you are running "
                                << OSRM_VERSION;
            util::Log(logERROR) << "Data is only compatible between minor releases.";
            return false;
        }

        return true;
    }

    std::size_t Size()
    {
        auto current_pos = input_stream.tellg();
        input_stream.seekg(0, input_stream.end);
        auto length = input_stream.tellg();
        input_stream.seekg(current_pos, input_stream.beg);
        return length;
    }

    std::vector<std::string> ReadLines()
    {
        std::vector<std::string> result;
        std::string thisline;
        try
        {
            while (std::getline(input_stream, thisline))
            {
                result.push_back(thisline);
            }
        }
        catch (const std::ios_base::failure &)
        {
            // EOF is OK here, everything else, re-throw
            if (!input_stream.eof())
                throw;
        }
        return result;
    }

    std::string ReadLine()
    {
        std::string thisline;
        try
        {
            std::getline(input_stream, thisline);
        }
        catch (const std::ios_base::failure & /*e*/)
        {
            // EOF is OK here, everything else, re-throw
            if (!input_stream.eof())
                throw;
        }
        return thisline;
    }
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
            throw util::exception("Error opening " + filepath.string());

        if (flag == GenerateFingerprint)
        {
            WriteFingerprint();
        }
    }

    /* Write count objects of type T from pointer src to output stream */
    template <typename T> void WriteFrom(const T *src, const std::size_t count)
    {
#if not defined __GNUC__ or __GNUC__ > 4
        static_assert(std::is_trivially_copyable<T>::value,
                      "bytewise writing requires trivially copyable type");
#endif

        if (count == 0)
            return;

        const auto &result =
            output_stream.write(reinterpret_cast<const char *>(src), count * sizeof(T));

        if (!result)
        {
            throw util::exception("Error writing to " + filepath.string());
        }
    }

    template <typename T> void WriteFrom(const T &target) { WriteFrom(&target, 1); }

    template <typename T> void WriteOne(const T tmp) { WriteFrom(tmp); }

    void WriteElementCount32(const std::uint32_t count) { WriteOne<std::uint32_t>(count); }
    void WriteElementCount64(const std::uint64_t count) { WriteOne<std::uint64_t>(count); }

    template <typename T> void SerializeVector(const std::vector<T> &data)
    {
        const auto count = data.size();
        WriteElementCount64(count);
        return WriteFrom(data.data(), count);
    }

    void WriteFingerprint()
    {
        const auto fingerprint = util::FingerPrint::GetValid();
        return WriteOne(fingerprint);
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
} // ns io
} // ns storage
} // ns osrm

#endif
