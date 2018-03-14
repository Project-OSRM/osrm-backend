#ifndef OSRM_STORAGE_TAR_HPP
#define OSRM_STORAGE_TAR_HPP

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/fingerprint.hpp"
#include "util/version.hpp"

#include <boost/filesystem/path.hpp>

extern "C" {
#include "microtar.h"
}

namespace osrm
{
namespace storage
{

class TarFileReader
{
  public:
    TarFileReader(const boost::filesystem::path &path) : path(path)
    {
        mtar_open(&handle, path.c_str(), "r");
    }

    template <typename T> T ReadOne(const std::string &name)
    {
        mtar_header_t header;
        mtar_find(&handle, name.c_str(), &header);
        if (header.size != sizeof(T))
        {
            throw util::exception("Datatype size does not match file size.");
        }

        T tmp;
        mtar_read_data(&handle, reinterpret_cast<char *>(&tmp), header.size);
        return tmp;
    }

    template <typename T>
    void ReadInto(const std::string &name, T &data, const std::size_t number_of_entries)
    {
        mtar_header_t header;
        mtar_find(&handle, name.c_str(), &header);
        if (header.size != sizeof(T) * number_of_entries)
        {
            throw util::exception("Datatype size does not match file size.");
        }

        mtar_read_data(&handle, reinterpret_cast<char *>(&data), header.size);
    }

    using TarEntry = std::tuple<std::string, std::size_t>;
    template <typename OutIter> void List(OutIter out)
    {
        mtar_header_t header;
        while ((mtar_read_header(&handle, &header)) != MTAR_ENULLRECORD)
        {
            *out++ = std::tuple<std::string, std::uint64_t>(header.name, header.size);
        }
    }

  private:
    bool ReadAndCheckFingerprint()
    {
        auto loaded_fingerprint = ReadOne<util::FingerPrint>("osrm_fingerprint");
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

class TarFileWriter
{
  public:
    TarFileWriter(const boost::filesystem::path &path) : path(path)
    {
        mtar_open(&handle, path.c_str(), "w");
        WriteFingerprint();
    }

    template <typename T> void WriteOne(const std::string &name, const T &data)
    {
        mtar_write_file_header(&handle, name.c_str(), name.size());
        mtar_write_data(&handle, reinterpret_cast<const char *>(&data), sizeof(T));
    }

    template <typename T>
    void WriteFrom(const std::string &name, const T &data, const std::size_t number_of_entries)
    {
        mtar_write_file_header(&handle, name.c_str(), name.size());
        mtar_write_data(&handle, reinterpret_cast<const char *>(&data), number_of_entries * sizeof(T));
    }

  private:
    void WriteFingerprint()
    {
        const auto fingerprint = util::FingerPrint::GetValid();
        WriteOne("osrm_fingerprint", fingerprint);
    }

    boost::filesystem::path path;
    mtar_t handle;
};
}
}

#endif
