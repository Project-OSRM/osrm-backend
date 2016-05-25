#ifndef OSRM_INCLUDE_UTIL_IO_HPP_
#define OSRM_INCLUDE_UTIL_IO_HPP_

#include "util/simple_logger.hpp"

#include <boost/filesystem.hpp>

#include <cstddef>
#include <cstdint>

#include <bitset>
#include <fstream>
#include <vector>

#include "util/fingerprint.hpp"

namespace osrm
{
namespace util
{

inline bool writeFingerprint(std::ostream &stream)
{
    const auto fingerprint = FingerPrint::GetValid();
    stream.write(reinterpret_cast<const char *>(&fingerprint), sizeof(fingerprint));
    return static_cast<bool>(stream);
}

inline bool readAndCheckFingerprint(std::istream &stream)
{
    FingerPrint fingerprint;
    const auto valid = FingerPrint::GetValid();
    stream.read(reinterpret_cast<char *>(&fingerprint), sizeof(fingerprint));
    // compare the compilation state stored in the fingerprint
    return static_cast<bool>(stream) && valid.IsMagicNumberOK(fingerprint) &&
           valid.TestContractor(fingerprint) && valid.TestGraphUtil(fingerprint) &&
           valid.TestRTree(fingerprint) && valid.TestQueryObjects(fingerprint);
}

template <typename simple_type>
bool serializeVector(const std::string &filename, const std::vector<simple_type> &data)
{
    std::ofstream stream(filename, std::ios::binary);

    writeFingerprint(stream);

    std::uint64_t count = data.size();
    stream.write(reinterpret_cast<const char *>(&count), sizeof(count));
    if (!data.empty())
        stream.write(reinterpret_cast<const char *>(&data[0]), sizeof(simple_type) * count);
    return static_cast<bool>(stream);
}

template <typename simple_type>
bool serializeVector(std::ostream &stream, const std::vector<simple_type> &data)
{
    std::uint64_t count = data.size();
    stream.write(reinterpret_cast<const char *>(&count), sizeof(count));
    if (!data.empty())
        stream.write(reinterpret_cast<const char *>(&data[0]), sizeof(simple_type) * count);
    return static_cast<bool>(stream);
}

template <typename simple_type>
bool deserializeVector(const std::string &filename, std::vector<simple_type> &data)
{
    std::ifstream stream(filename, std::ios::binary);

    if (!readAndCheckFingerprint(stream))
        return false;

    std::uint64_t count = 0;
    stream.read(reinterpret_cast<char *>(&count), sizeof(count));
    data.resize(count);
    if (count)
        stream.read(reinterpret_cast<char *>(&data[0]), sizeof(simple_type) * count);
    return static_cast<bool>(stream);
}

template <typename simple_type>
bool deserializeVector(std::istream &stream, std::vector<simple_type> &data)
{
    std::uint64_t count = 0;
    stream.read(reinterpret_cast<char *>(&count), sizeof(count));
    data.resize(count);
    if (count)
        stream.read(reinterpret_cast<char *>(&data[0]), sizeof(simple_type) * count);
    return static_cast<bool>(stream);
}

inline bool serializeFlags(const boost::filesystem::path &path, const std::vector<bool> &flags)
{
    // TODO this should be replaced with a FILE-based write using error checking
    std::ofstream flag_stream(path.string(), std::ios::binary);

    writeFingerprint(flag_stream);

    std::uint32_t number_of_bits = flags.size();
    flag_stream.write(reinterpret_cast<const char *>(&number_of_bits), sizeof(number_of_bits));
    // putting bits in ints
    std::uint32_t chunk = 0;
    std::size_t chunk_count = 0;
    for (std::size_t bit_nr = 0; bit_nr < number_of_bits;)
    {
        std::bitset<32> chunk_bitset;
        for (std::size_t chunk_bit = 0; chunk_bit < 32 && bit_nr < number_of_bits;
             ++chunk_bit, ++bit_nr)
            chunk_bitset[chunk_bit] = flags[bit_nr];

        chunk = chunk_bitset.to_ulong();
        ++chunk_count;
        flag_stream.write(reinterpret_cast<const char *>(&chunk), sizeof(chunk));
    }
    SimpleLogger().Write() << "Wrote " << number_of_bits << " bits in " << chunk_count
                           << " chunks (Flags).";
    return static_cast<bool>(flag_stream);
}

inline bool deserializeFlags(const boost::filesystem::path &path, std::vector<bool> &flags)
{
    SimpleLogger().Write() << "Reading flags from " << path;
    std::ifstream flag_stream(path.string(), std::ios::binary);

    if (!readAndCheckFingerprint(flag_stream))
        return false;

    std::uint32_t number_of_bits;
    flag_stream.read(reinterpret_cast<char *>(&number_of_bits), sizeof(number_of_bits));
    flags.resize(number_of_bits);
    // putting bits in ints
    std::uint32_t chunks = (number_of_bits + 31) / 32;
    std::size_t bit_position = 0;
    std::uint32_t chunk;
    for (std::size_t chunk_id = 0; chunk_id < chunks; ++chunk_id)
    {
        flag_stream.read(reinterpret_cast<char *>(&chunk), sizeof(chunk));
        std::bitset<32> chunk_bits(chunk);
        for (std::size_t bit = 0; bit < 32 && bit_position < number_of_bits; ++bit, ++bit_position)
            flags[bit_position] = chunk_bits[bit];
    }
    SimpleLogger().Write() << "Read " << number_of_bits << " bits in " << chunks
                           << " Chunks from disk.";
    return static_cast<bool>(flag_stream);
}
} // namespace util
} // namespace osrm

#endif // OSRM_INCLUDE_UTIL_IO_HPP_
