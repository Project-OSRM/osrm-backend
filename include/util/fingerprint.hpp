#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <array>
#include <boost/uuid/uuid.hpp>
#include <cstdint>
#include <type_traits>

namespace osrm
{
namespace util
{

// implements a singleton, i.e. there is one and only one conviguration object
struct FingerPrint
{
    static FingerPrint GetValid();

    bool IsValid() const;
    bool IsDataCompatible(const FingerPrint &other) const;

    int GetMajorVersion() const;
    int GetMinorVersion() const;
    int GetPatchVersion() const;

  private:
    std::uint8_t CalculateChecksum() const;
    // Here using std::array so that == can be used to conveniently compare contents
    std::array<std::uint8_t, 4> magic_number;
    std::uint8_t major_version;
    std::uint8_t minor_version;
    std::uint8_t patch_version;
    std::uint8_t checksum; // CRC8 of the previous bytes to ensure the fingerprint is not damaged
};

static_assert(sizeof(FingerPrint) == 8, "FingerPrint has unexpected size");
static_assert(std::is_trivial<FingerPrint>::value, "FingerPrint needs to be trivial.");
static_assert(std::is_pod<FingerPrint>::value, "FingerPrint needs to be a POD.");
} // namespace util
} // namespace osrm

#endif /* FingerPrint_H */
