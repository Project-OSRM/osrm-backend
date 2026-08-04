#include "util/fingerprint.hpp"

#include "storage/io.hpp"
#include "storage/tar.hpp"
#include "util/exception.hpp"
#include "util/version.hpp"

#include <boost/crc.hpp>
#include <boost/test/unit_test.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

using namespace osrm;

namespace
{
constexpr std::size_t FP_SIZE = sizeof(util::FingerPrint);

// Byte offsets within the standard-layout FingerPrint:
//   [0..3] magic  [4] major  [5] minor  [6] patch
//   [7] os  [8] endianness  [9] pointer_bytes  [10] checksum
// `checksum` is guaranteed last by a static_assert in fingerprint.cpp, and the
// struct is packed (all 1-byte fields), so these offsets are stable.
static_assert(FP_SIZE == 11, "Test assumes 11-byte fingerprint layout");
constexpr std::size_t OS_OFFSET = 7;
constexpr std::size_t ENDIAN_OFFSET = 8;
constexpr std::size_t POINTER_OFFSET = 9;
constexpr std::size_t CHECKSUM_OFFSET = 10;

using Bytes = std::array<std::uint8_t, FP_SIZE>;

Bytes toBytes(const util::FingerPrint &fp)
{
    Bytes bytes{};
    std::memcpy(bytes.data(), &fp, FP_SIZE);
    return bytes;
}

util::FingerPrint fromBytes(const Bytes &bytes)
{
    util::FingerPrint fp;
    std::memcpy(&fp, bytes.data(), FP_SIZE);
    return fp;
}

// Mirrors FingerPrint::CalculateChecksum (CRC-8-CCITT, poly 0x07) so the test
// can produce fingerprints that still pass IsValid() after patching ABI bytes.
std::uint8_t crc8(const Bytes &bytes)
{
    boost::crc_optimal<8, 0x07> crc;
    crc.process_bytes(bytes.data(), CHECKSUM_OFFSET);
    return crc.checksum();
}

// Build a valid fingerprint for the current version but with the ABI descriptor
// overridden, repairing the checksum so IsValid() still holds.
util::FingerPrint makeWithABI(std::uint8_t os, std::uint8_t endianness, std::uint8_t pointer_bytes)
{
    auto bytes = toBytes(util::FingerPrint::GetValid());
    bytes[OS_OFFSET] = os;
    bytes[ENDIAN_OFFSET] = endianness;
    bytes[POINTER_OFFSET] = pointer_bytes;
    bytes[CHECKSUM_OFFSET] = crc8(bytes);
    return fromBytes(bytes);
}

std::uint8_t rawByte(const util::FingerPrint &fp, std::size_t offset)
{ return toBytes(fp)[offset]; }
} // namespace

BOOST_AUTO_TEST_SUITE(fingerprint)

BOOST_AUTO_TEST_CASE(valid_fingerprint_describes_current_platform)
{
    const auto fp = util::FingerPrint::GetValid();

    BOOST_CHECK(fp.IsValid());
    // A fingerprint is always data-compatible with itself.
    BOOST_CHECK(fp.IsDataCompatible(fp));
    BOOST_CHECK(fp.IsABICompatible(fp));

    BOOST_CHECK_EQUAL(fp.GetMajorVersion(), OSRM_VERSION_MAJOR);
    BOOST_CHECK_EQUAL(fp.GetMinorVersion(), OSRM_VERSION_MINOR);
    BOOST_CHECK_EQUAL(fp.GetPatchVersion(), OSRM_VERSION_PATCH);

    // GetValid() must record a concrete ABI, never the Unknown sentinel.
    BOOST_CHECK(fp.GetOperatingSystem() != util::FingerPrint::OperatingSystem::Unknown);
    BOOST_CHECK_EQUAL(fp.GetPointerBytes(), static_cast<int>(sizeof(void *)));
    BOOST_CHECK(std::string(fp.GetOperatingSystemString()) != "unknown");
    BOOST_CHECK(std::string(fp.GetEndiannessString()) != "unknown-endian");
}

BOOST_AUTO_TEST_CASE(abi_mismatch_breaks_compatibility)
{
    const auto current = util::FingerPrint::GetValid();
    const auto os = rawByte(current, OS_OFFSET);
    const auto endian = rawByte(current, ENDIAN_OFFSET);
    const auto pointer = rawByte(current, POINTER_OFFSET);

    // Same version and same ABI => compatible.
    const auto same = makeWithABI(os, endian, pointer);
    BOOST_CHECK(same.IsValid());
    BOOST_CHECK(current.IsABICompatible(same));
    BOOST_CHECK(current.IsDataCompatible(same));

    // Differing OS, endianness or pointer width each break ABI (and data)
    // compatibility, even though the version bytes still match.
    const auto other_os = makeWithABI(static_cast<std::uint8_t>(os == 1 ? 2 : 1), endian, pointer);
    BOOST_CHECK(other_os.IsValid());
    BOOST_CHECK(!current.IsABICompatible(other_os));
    BOOST_CHECK(!current.IsDataCompatible(other_os));

    const auto other_endian =
        makeWithABI(os, static_cast<std::uint8_t>(endian == 1 ? 2 : 1), pointer);
    BOOST_CHECK(!current.IsABICompatible(other_endian));
    BOOST_CHECK(!current.IsDataCompatible(other_endian));

    const auto other_pointer =
        makeWithABI(os, endian, static_cast<std::uint8_t>(pointer == 8 ? 4 : 8));
    BOOST_CHECK(!current.IsABICompatible(other_pointer));
    BOOST_CHECK(!current.IsDataCompatible(other_pointer));
}

BOOST_AUTO_TEST_CASE(operating_system_string_covers_all_values)
{
    const auto current = util::FingerPrint::GetValid();
    const auto endian = rawByte(current, ENDIAN_OFFSET);
    const auto pointer = rawByte(current, POINTER_OFFSET);

    const struct
    {
        std::uint8_t value;
        const char *text;
    } cases[] = {
        {0, "unknown"}, // Unknown
        {1, "Linux"},
        {2, "Windows"},
        {3, "macOS"},
        {4, "FreeBSD"},
        {255, "other"},   // Other
        {100, "unknown"}, // unmapped value falls through to the default arm
    };

    for (const auto &c : cases)
    {
        const auto fp = makeWithABI(c.value, endian, pointer);
        BOOST_CHECK_EQUAL(std::string(fp.GetOperatingSystemString()), std::string(c.text));
        BOOST_CHECK_EQUAL(static_cast<int>(rawByte(fp, OS_OFFSET)), static_cast<int>(c.value));
    }
}

BOOST_AUTO_TEST_CASE(endianness_string_covers_all_values)
{
    const auto current = util::FingerPrint::GetValid();
    const auto os = rawByte(current, OS_OFFSET);
    const auto pointer = rawByte(current, POINTER_OFFSET);

    const struct
    {
        std::uint8_t value;
        const char *text;
    } cases[] = {
        {0, "unknown-endian"}, // Unknown
        {1, "little-endian"},
        {2, "big-endian"},
        {99, "unknown-endian"}, // unmapped value falls through to the default arm
    };

    for (const auto &c : cases)
    {
        const auto fp = makeWithABI(os, c.value, pointer);
        BOOST_CHECK_EQUAL(std::string(fp.GetEndiannessString()), std::string(c.text));
    }
}

BOOST_AUTO_TEST_CASE(accessors_reflect_patched_descriptor)
{
    const auto fp =
        makeWithABI(static_cast<std::uint8_t>(util::FingerPrint::OperatingSystem::Windows),
                    static_cast<std::uint8_t>(util::FingerPrint::Endianness::Big),
                    4);
    BOOST_CHECK(fp.GetOperatingSystem() == util::FingerPrint::OperatingSystem::Windows);
    BOOST_CHECK_EQUAL(fp.GetPointerBytes(), 4);
    BOOST_CHECK_EQUAL(std::string(fp.GetEndiannessString()), std::string("big-endian"));
}

// A dataset produced on a different OS/ABI must be rejected by the io.hpp reader
// with a descriptive IncompatibleFileVersion error, not silently misread (#4404).
BOOST_AUTO_TEST_CASE(io_reader_rejects_foreign_abi)
{
    const std::string file = "foreign_abi_io.tmp";
    const auto current = util::FingerPrint::GetValid();
    const auto os = rawByte(current, OS_OFFSET);
    const auto foreign = makeWithABI(static_cast<std::uint8_t>(os == 1 ? 2 : 1),
                                     rawByte(current, ENDIAN_OFFSET),
                                     rawByte(current, POINTER_OFFSET));

    {
        storage::io::FileWriter out(file, storage::io::FileWriter::HasNoFingerprint);
        out.WriteFrom(foreign);
    }

    try
    {
        storage::io::FileReader in(file, storage::io::FileReader::VerifyFingerprint);
        BOOST_REQUIRE_MESSAGE(false, "Reader should have rejected the foreign-ABI fingerprint");
    }
    catch (const util::RuntimeError &e)
    {
        BOOST_CHECK(e.GetCode() == ErrorCode::IncompatibleFileVersion);
        const std::string got(e.what());
        BOOST_CHECK(got.find("was prepared on") != std::string::npos);
        BOOST_CHECK(got.find("not portable across platforms") != std::string::npos);
    }
}

// Same check for the tar reader, which stores the fingerprint under a named entry.
BOOST_AUTO_TEST_CASE(tar_reader_rejects_foreign_abi)
{
    const std::string file = "foreign_abi_tar.tmp";
    const auto current = util::FingerPrint::GetValid();
    const auto os = rawByte(current, OS_OFFSET);
    const auto foreign = makeWithABI(static_cast<std::uint8_t>(os == 1 ? 2 : 1),
                                     rawByte(current, ENDIAN_OFFSET),
                                     rawByte(current, POINTER_OFFSET));

    {
        storage::tar::FileWriter out(file, storage::tar::FileWriter::HasNoFingerprint);
        out.WriteFrom("osrm_fingerprint.meta", foreign);
    }

    try
    {
        storage::tar::FileReader in(file, storage::tar::FileReader::VerifyFingerprint);
        BOOST_REQUIRE_MESSAGE(false, "Reader should have rejected the foreign-ABI fingerprint");
    }
    catch (const util::RuntimeError &e)
    {
        BOOST_CHECK(e.GetCode() == ErrorCode::IncompatibleFileVersion);
        const std::string got(e.what());
        BOOST_CHECK(got.find("was prepared on") != std::string::npos);
        BOOST_CHECK(got.find("not portable across platforms") != std::string::npos);
    }
}

BOOST_AUTO_TEST_SUITE_END()
