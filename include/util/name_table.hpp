#ifndef OSRM_UTIL_NAME_TABLE_HPP
#define OSRM_UTIL_NAME_TABLE_HPP

#include "util/indexed_data.hpp"
#include "util/shared_memory_vector_wrapper.hpp"
#include "util/string_view.hpp"
#include "util/typedefs.hpp"

#include <string>

namespace osrm
{
namespace util
{

// While this could, theoretically, hold any names in the fitting format,
// the NameTable allows access to a part of the Datafacade to allow
// processing based on name indices.
class NameTable
{
  public:
    using IndexedData = util::IndexedData<util::VariableGroupBlock<16, util::StringView>>;
    using ResultType = IndexedData::ResultType;
    using ValueType = IndexedData::ValueType;

    NameTable() {}

    // Read filename and store own data in m_buffer
    NameTable(const std::string &filename);

    // Keep pointers only in m_name_table and don't own data in m_buffer
    void reset(ValueType *begin, ValueType *end);

    // This class provides a limited view over all the string data we serialize out.
    // The following functions are a subset of what is available.
    // See the data facades for they provide full access to this serialized string data.
    // (at time of writing this: get{Name,Ref,Pronunciation,Destinations}ForID(name_id);)
    util::StringView GetNameForID(const NameID id) const;
    util::StringView GetDestinationsForID(const NameID id) const;
    util::StringView GetRefForID(const NameID id) const;
    util::StringView GetPronunciationForID(const NameID id) const;

  private:
    using BufferType = std::unique_ptr<ValueType, std::function<void(void *)>>;

    BufferType m_buffer;
    IndexedData m_name_table;
};
} // namespace util
} // namespace osrm

#endif // OSRM_UTIL_NAME_TABLE_HPP
