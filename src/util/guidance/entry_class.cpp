#include "util/guidance/entry_class.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace util
{
namespace guidance
{

EntryClass::EntryClass() : enabled_entries_flags(0) {}

void EntryClass::activate(std::uint32_t index)
{
    BOOST_ASSERT(index < 8 * sizeof(FlagBaseType));
    enabled_entries_flags |= (1 << index);
}

bool EntryClass::allowsEntry(std::uint32_t index) const
{
    BOOST_ASSERT(index < 8 * sizeof(FlagBaseType));
    return 0 != (enabled_entries_flags & (1 << index));
}

bool EntryClass::operator==(const EntryClass &other) const
{
    return enabled_entries_flags == other.enabled_entries_flags;
}

bool EntryClass::operator<(const EntryClass &other) const
{
    return enabled_entries_flags < other.enabled_entries_flags;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
