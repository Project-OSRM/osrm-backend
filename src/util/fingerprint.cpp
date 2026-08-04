#include "util/fingerprint.hpp"
#include "util/version.hpp"

#include <boost/crc.hpp>

#include <bit>
#include <cstddef>

namespace osrm::util
{

namespace
{
// Detect the operating system / ABI family of the build that is running.
constexpr FingerPrint::OperatingSystem DetectOperatingSystem()
{
#if defined(_WIN32)
    return FingerPrint::OperatingSystem::Windows;
#elif defined(__APPLE__)
    return FingerPrint::OperatingSystem::macOS;
#elif defined(__linux__)
    return FingerPrint::OperatingSystem::Linux;
#elif defined(__FreeBSD__)
    return FingerPrint::OperatingSystem::FreeBSD;
#else
    return FingerPrint::OperatingSystem::Other;
#endif
}

constexpr FingerPrint::Endianness DetectEndianness()
{
    if constexpr (std::endian::native == std::endian::little)
    {
        return FingerPrint::Endianness::Little;
    }
    else if constexpr (std::endian::native == std::endian::big)
    {
        return FingerPrint::Endianness::Big;
    }
    else
    {
        return FingerPrint::Endianness::Unknown;
    }
}
} // namespace

/**
 * Constructs a valid fingerprint for the current (running) version of OSRM.
 * This can be compared to one read from a file to determine whether the
 * current code is compatible with the file being read.
 */
FingerPrint FingerPrint::GetValid()
{
    FingerPrint fingerprint;

    // 4 chars magic number. Bump the final char whenever the fingerprint format
    // or on-disk data layout changes, to force incompatibility with datasets
    // produced by older code.
    //   'M' -> v1, 'N' -> semver scheme, 'O' -> packed_osm_ids layout,
    //   'P' -> added ABI descriptor (os / endianness / pointer size).
    fingerprint.magic_number = {{'O', 'S', 'R', 'P'}};
    fingerprint.major_version = OSRM_VERSION_MAJOR;
    fingerprint.minor_version = OSRM_VERSION_MINOR;
    fingerprint.patch_version = OSRM_VERSION_PATCH;
    fingerprint.os = DetectOperatingSystem();
    fingerprint.endianness = DetectEndianness();
    fingerprint.pointer_bytes = static_cast<std::uint8_t>(sizeof(void *));
    fingerprint.checksum = fingerprint.CalculateChecksum();

    return fingerprint;
}

int FingerPrint::GetMajorVersion() const { return major_version; }
int FingerPrint::GetMinorVersion() const { return minor_version; }
int FingerPrint::GetPatchVersion() const { return patch_version; }

FingerPrint::OperatingSystem FingerPrint::GetOperatingSystem() const { return os; }
int FingerPrint::GetPointerBytes() const { return pointer_bytes; }

const char *FingerPrint::GetOperatingSystemString() const
{
    switch (os)
    {
    case OperatingSystem::Linux:
        return "Linux";
    case OperatingSystem::Windows:
        return "Windows";
    case OperatingSystem::macOS:
        return "macOS";
    case OperatingSystem::FreeBSD:
        return "FreeBSD";
    case OperatingSystem::Other:
        return "other";
    case OperatingSystem::Unknown:
    default:
        return "unknown";
    }
}

const char *FingerPrint::GetEndiannessString() const
{
    switch (endianness)
    {
    case Endianness::Little:
        return "little-endian";
    case Endianness::Big:
        return "big-endian";
    case Endianness::Unknown:
    default:
        return "unknown-endian";
    }
}

/**
 * Calculates the CRC8 of the FingerPrint struct, using all bytes except the
 * final `checksum` field, which should be last in the struct (this function
 * checks that it is)
 */
std::uint8_t FingerPrint::CalculateChecksum() const
{
    // Verify that the checksum is a single byte (because we're returning an 8 bit checksum)
    // This assumes that a byte == 8 bits, which is mostly true these days unless you're doing
    // something really weird
    static_assert(sizeof(checksum) == 1, "Checksum needs to be a single byte");
    const constexpr int CRC_BITS = 8;

    // This constant comes from
    // https://en.wikipedia.org/wiki/Polynomial_representations_of_cyclic_redundancy_checks
    // CRC-8-CCITT normal polynomial value.
    const constexpr int CRC_POLYNOMIAL = 0x07;
    boost::crc_optimal<CRC_BITS, CRC_POLYNOMIAL> crc8;

    // Verify that the checksum is the last field, because we're going to CRC all the bytes
    // leading up to it
    static_assert(offsetof(FingerPrint, checksum) == sizeof(FingerPrint) - sizeof(checksum),
                  "Checksum must be the final field in the Fingerprint struct");

    // Calculate checksum of all bytes except the checksum byte, which is at the end.
    crc8.process_bytes(this, sizeof(FingerPrint) - sizeof(checksum));

    return crc8.checksum();
}

/**
 * Verifies that the fingerprint has the expected magic number, and the checksum is correct.
 */
bool FingerPrint::IsValid() const
{
    // Note: == on std::array compares contents, which is what we want here.
    return magic_number == GetValid().magic_number && checksum == CalculateChecksum();
}

/**
 * Determines whether `other` was produced by a binary-compatible OS/ABI, i.e.
 * its raw data image can be interpreted by the platform running this code.
 * The .osrm.* files are a raw image of in-memory structures, so any difference
 * in the producing platform's struct layout makes the data unusable here.
 */
bool FingerPrint::IsABICompatible(const FingerPrint &other) const
{ return other.os == os && other.endianness == endianness && other.pointer_bytes == pointer_bytes; }

/**
 * Determines whether two fingerprints are data compatible.
 * Our compatibility rules say that we maintain data compatibility for all PATCH versions.
 * A difference in either the MAJOR or MINOR version fields means the data is considered
 * incompatible, as does a difference in the producing platform's ABI.
 */
bool FingerPrint::IsDataCompatible(const FingerPrint &other) const
{
    return IsValid() && other.major_version == major_version &&
           other.minor_version == minor_version && IsABICompatible(other);
}
} // namespace osrm::util
