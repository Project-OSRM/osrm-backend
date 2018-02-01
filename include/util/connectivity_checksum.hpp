#ifndef CONNECTIVITY_CHECKSUM_HPP
#define CONNECTIVITY_CHECKSUM_HPP

#include <zlib.h>

#include <boost/assert.hpp>

#include <array>
#include <climits>
#include <cstdint>

namespace osrm
{
namespace util
{

struct ConnectivityChecksum
{
    ConnectivityChecksum() : checksum(0), length(0), byte_number(0), bit_number(0) {}

    void process_byte(unsigned char byte)
    {
        BOOST_ASSERT(byte_number < buffer.size());

        if (bit_number > 0)
        {
            bit_number = 0;
            ++byte_number;
            ++length;
        }
        flush_bytes();

        buffer[byte_number] = byte;
        ++byte_number;
        ++length;
        flush_bytes();

        buffer[byte_number] = 0;
    }

    void process_bit(bool bit)
    {
        BOOST_ASSERT(byte_number < buffer.size());
        BOOST_ASSERT(bit_number < CHAR_BIT);

        buffer[byte_number] = (buffer[byte_number] << 1) | static_cast<unsigned char>(bit);
        if (++bit_number >= CHAR_BIT)
        {
            bit_number = 0;
            ++byte_number;
            ++length;
            flush_bytes();
            buffer[byte_number] = 0;
        }
    }

    std::uint32_t update_checksum(std::uint32_t current)
    {
        if (bit_number > 0)
        {
            ++byte_number;
            ++length;
        }
        checksum = crc32(checksum, buffer.data(), byte_number);
        checksum = crc32_combine(current, checksum, length);
        length = byte_number = bit_number = 0;
        buffer[byte_number] = 0;
        return checksum;
    }

  private:
    void flush_bytes()
    {
        if (byte_number >= buffer.size())
        {
            checksum = crc32(checksum, buffer.data(), buffer.size());
            byte_number = 0;
        }
    }

    std::array<unsigned char, 64> buffer;
    std::uint32_t checksum;
    std::size_t length;
    std::size_t byte_number;
    unsigned char bit_number;
};
}
}

#endif
