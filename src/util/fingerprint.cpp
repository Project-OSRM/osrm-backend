#include "util/fingerprint.hpp"
#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/version.hpp"

#include <boost/assert.hpp>
#include <boost/crc.hpp>

#include <cstring>

#include <algorithm>
#include <string>

namespace osrm
{
namespace util
{

/**
 * Constructs a valid fingerprint for the current (running) version of OSRM.
 * This can be compared to one read from a file to determine whether the
 * current code is compatible with the file being read.
 */
FingerPrint FingerPrint::GetValid()
{
    FingerPrint fingerprint;

    // 4 chars, 'O','S','R','N' - note the N instead of M, v1 of the fingerprint
    // used M, so we add one and use N to indicate the newer fingerprint magic number.
    // Bump this value if the fingerprint format ever changes.
    fingerprint.magic_number = {{'O', 'S', 'R', 'N'}};
    fingerprint.major_version = OSRM_VERSION_MAJOR;
    fingerprint.minor_version = OSRM_VERSION_MINOR;
    fingerprint.patch_version = OSRM_VERSION_PATCH;
    fingerprint.checksum = fingerprint.CalculateChecksum();

    return fingerprint;
}

int FingerPrint::GetMajorVersion() const { return major_version; }
int FingerPrint::GetMinorVersion() const { return minor_version; }
int FingerPrint::GetPatchVersion() const { return patch_version; }

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
 * Determines whether two fingerprints are data compatible.
 * Our compatibility rules say that we maintain data compatibility for all PATCH versions.
 * A difference in either the MAJOR or MINOR version fields means the data is considered
 * incompatible.
 */
bool FingerPrint::IsDataCompatible(const FingerPrint &other) const
{
    return IsValid() && other.major_version == major_version &&
           other.minor_version == minor_version;
}
} // namespace util
} // namespace osrm
