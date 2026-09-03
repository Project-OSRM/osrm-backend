#ifndef OSRM_UTIL_GUIDANCE_ENTRY_CLASS_HPP_
#define OSRM_UTIL_GUIDANCE_ENTRY_CLASS_HPP_

#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace osrm::util::guidance
{
class EntryClass;
} // namespace osrm::util::guidance

namespace std
{
template <> struct hash<::osrm::util::guidance::EntryClass>
{
    inline std::size_t operator()(const ::osrm::util::guidance::EntryClass &entry_class) const;
};
} // namespace std

namespace osrm
{
namespace util::guidance
{

class EntryClass
{
    using FlagBaseType = std::uint64_t;

  public:
    /**
     * How many roads of one intersection this can hold an answer for.
     *
     * Sixty-four rather than thirty-two because a meshed pedestrian area produces
     * intersections far larger than a junction ever is: every line of sight from a plaza
     * vertex is a way. Over Ile-de-France 276 vertices had more roads than thirty-two
     * bits could describe, with a median of 38 and a worst of 151; sixty-four covers all
     * but eight of them. A road beyond this reports no entry -- see allowsEntry -- which
     * is the one answer the stored data supports.
     *
     * Widening again is cheap in space, since only the *distinct* classes are stored and
     * EntryClassID caps those at 65536, but it changes the layout of .osrm.icd and so
     * costs a re-extraction.
     */
    static constexpr std::uint32_t CAPACITY = CHAR_BIT * sizeof(FlagBaseType);

    constexpr EntryClass() : enabled_entries_flags(0) {}

    // we are hiding the access to the flags behind a protection wall, to make sure the bit logic
    // isn't tempered with. zero based indexing
    // return true if was activated and false if activation failed
    bool activate(std::uint32_t index);

    // check whether a certain turn allows entry
    bool allowsEntry(std::uint32_t index) const;

    // required for hashing
    bool operator==(const EntryClass &) const;

    // sorting
    bool operator<(const EntryClass &) const;

  private:
    // given a list of possible discrete angles, the available angles flag indicates the presence of
    // a given turn at the intersection
    FlagBaseType enabled_entries_flags;

    // allow hash access to internal representation
    friend std::size_t std::hash<EntryClass>::operator()(const EntryClass &) const;
};

#if !defined(__GNUC__) || (__GNUC__ > 4)
static_assert(std::is_trivially_copyable<EntryClass>::value,
              "Class is serialized trivially in "
              "the datafacades. Bytewise writing "
              "requires trivially copyable type");
#endif

} // namespace util::guidance

constexpr const util::guidance::EntryClass EMPTY_ENTRY_CLASS{};
} // namespace osrm

// make Entry Class hasbable
namespace std
{
inline size_t hash<::osrm::util::guidance::EntryClass>::operator()(
    const ::osrm::util::guidance::EntryClass &entry_class) const
{
    return hash<::osrm::util::guidance::EntryClass::FlagBaseType>()(
        entry_class.enabled_entries_flags);
}
} // namespace std

#endif /* OSRM_UTIL_GUIDANCE_ENTRY_CLASS_HPP_ */
