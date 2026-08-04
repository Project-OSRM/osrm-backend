#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <array>
#include <cstdint>
#include <type_traits>

namespace osrm::util
{

// implements a singleton, i.e. there is one and only one conviguration object
struct FingerPrint
{
    // Identifies the operating system / ABI family that produced a dataset.
    // The .osrm.* files are a raw byte image of in-memory structures whose
    // layout (struct padding, and especially implementation-defined bitfield
    // ordering) differs across OS/compiler ABIs. A dataset produced on one
    // platform therefore cannot be safely reinterpreted on another. See #4404.
    // `Unknown` is the zero/sentinel value; it is never emitted by GetValid() and
    // only appears on a zeroed/malformed fingerprint. `Other` is emitted when the
    // producing platform was detected but is not one of the enumerated systems.
    enum class OperatingSystem : std::uint8_t
    {
        Unknown = 0,
        Linux = 1,
        Windows = 2,
        macOS = 3,
        FreeBSD = 4,
        Other = 255
    };

    enum class Endianness : std::uint8_t
    {
        Unknown = 0,
        Little = 1,
        Big = 2
    };

    static FingerPrint GetValid();

    bool IsValid() const;
    bool IsDataCompatible(const FingerPrint &other) const;
    // True when `other` was produced by a binary-compatible OS/ABI, i.e. its
    // raw data image can be interpreted by the platform running this code.
    bool IsABICompatible(const FingerPrint &other) const;

    int GetMajorVersion() const;
    int GetMinorVersion() const;
    int GetPatchVersion() const;

    OperatingSystem GetOperatingSystem() const;
    const char *GetOperatingSystemString() const;
    const char *GetEndiannessString() const;
    int GetPointerBytes() const;

  private:
    std::uint8_t CalculateChecksum() const;
    // Here using std::array so that == can be used to conveniently compare contents
    std::array<std::uint8_t, 4> magic_number;
    std::uint8_t major_version;
    std::uint8_t minor_version;
    std::uint8_t patch_version;
    // --- ABI descriptor: describes the platform that produced the data (#4404) ---
    OperatingSystem os;
    Endianness endianness;
    std::uint8_t pointer_bytes; // sizeof(void*) on the producing platform
    std::uint8_t checksum; // CRC8 of the previous bytes to ensure the fingerprint is not damaged
};

static_assert(sizeof(FingerPrint) == 11, "FingerPrint has unexpected size");
static_assert(std::is_trivially_default_constructible<FingerPrint>::value,
              "FingerPrint needs to be trivially default constructible.");
static_assert(std::is_trivially_copyable<FingerPrint>::value,
              "FingerPrint needs to be trivially copyable.");
static_assert(std::is_standard_layout<FingerPrint>::value,
              "FingerPrint needs to have a standard layout.");
} // namespace osrm::util

#endif /* FingerPrint_H */
