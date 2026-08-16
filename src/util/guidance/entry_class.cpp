#include "util/guidance/entry_class.hpp"

#include <boost/assert.hpp>

#include <climits>

namespace osrm::util::guidance
{

bool EntryClass::activate(std::uint32_t index)
{
    if (index >= CHAR_BIT * sizeof(FlagBaseType))
        return false;

    enabled_entries_flags |= (FlagBaseType{1} << index);
    return true;
}

bool EntryClass::allowsEntry(std::uint32_t index) const
{
    // Answered rather than asserted, because the caller cannot avoid asking.
    //
    // A road beyond the ones this class can hold was never stored: activate() refuses it
    // and the extractor logs that it did.  The engine, though, walks the bearings, and
    // there can be more of those than there are bits here.  An ordinary junction never
    // gets near the limit, but a meshed pedestrian area does: every line of sight from a
    // plaza vertex is a way, and a vertex on a busy plaza has been seen with 96 of them.
    //
    // Shifting by the width of the type is undefined, so with asserts on this aborted the
    // server and with them off it read whatever the shift happened to produce.  Neither
    // is an answer.  A road that could not be recorded reports no entry, which is what
    // the stored data says about it.
    if (index >= CHAR_BIT * sizeof(FlagBaseType))
        return false;

    return 0 != (enabled_entries_flags & (FlagBaseType{1} << index));
}

bool EntryClass::operator==(const EntryClass &other) const
{ return enabled_entries_flags == other.enabled_entries_flags; }

bool EntryClass::operator<(const EntryClass &other) const
{ return enabled_entries_flags < other.enabled_entries_flags; }

} // namespace osrm::util::guidance
