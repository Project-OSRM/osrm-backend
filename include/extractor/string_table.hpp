#ifndef OSRM_EXTRACTOR_STRING_TABLE_HPP
#define OSRM_EXTRACTOR_STRING_TABLE_HPP

#include "util/indexed_data.hpp"
#include "util/typedefs.hpp"

#include <string>
#include <string_view>

namespace osrm::extractor
{

namespace detail
{
template <storage::Ownership Ownership> class StringTableImpl;
} // namespace detail

namespace serialization
{
template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::StringTableImpl<Ownership> &index_data);

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::StringTableImpl<Ownership> &index_data);
} // namespace serialization

namespace detail
{
// This class provides a limited view over all the string data we serialize out.
// The following functions are a subset of what is available.
// See the data facades for they provide full access to this serialized string data.
// Way string data is stored in blocks based on `id` as follows:
//
// | name | destination | pronunciation | ref | exits
//                      ^               ^
//                      [range)
//                       ^ id + 2
//
// `id + offset` gives us the range of chars.
//
// Offset 0 is name, 1 is destination, 2 is pronunciation, 3 is ref, 4 is exits
// See datafacades and extractor callbacks for details.
template <storage::Ownership Ownership> class StringTableImpl
{
  public:
    using IndexedData =
        util::detail::IndexedDataImpl<util::VariableGroupBlock<16, std::string_view>, Ownership>;
    using ResultType = typename IndexedData::ResultType;
    using ValueType = typename IndexedData::ValueType;

    StringTableImpl() {}

    StringTableImpl(IndexedData indexed_data_) : indexed_data{std::move(indexed_data_)} {}

    std::string_view GetNameForID(const StringViewID id) const
    {
        if (id == INVALID_STRINGVIEWID)
            return {};

        return indexed_data.at(id + 0);
    }

    std::string_view GetDestinationsForID(const StringViewID id) const
    {
        if (id == INVALID_STRINGVIEWID)
            return {};

        return indexed_data.at(id + 1);
    }

    std::string_view GetExitsForID(const StringViewID id) const
    {
        if (id == INVALID_STRINGVIEWID)
            return {};

        return indexed_data.at(id + 4);
    }

    std::string_view GetRefForID(const StringViewID id) const
    {
        if (id == INVALID_STRINGVIEWID)
            return {};

        const constexpr auto OFFSET_REF = 3u;
        return indexed_data.at(id + OFFSET_REF);
    }

    std::string_view GetPronunciationForID(const StringViewID id) const
    {
        if (id == INVALID_STRINGVIEWID)
            return {};

        const constexpr auto OFFSET_PRONUNCIATION = 2u;
        return indexed_data.at(id + OFFSET_PRONUNCIATION);
    }

    friend void serialization::read<Ownership>(storage::tar::FileReader &reader,
                                               const std::string &name,
                                               StringTableImpl &index_data);

    friend void serialization::write<Ownership>(storage::tar::FileWriter &writer,
                                                const std::string &name,
                                                const StringTableImpl &index_data);

  private:
    IndexedData indexed_data;
};
} // namespace detail

using StringTable = detail::StringTableImpl<storage::Ownership::Container>;
using StringTableView = detail::StringTableImpl<storage::Ownership::View>;

// Backwards compatibility aliases (deprecated)
using NameTable = StringTable;
using NameTableView = StringTableView;
using PreDatafacadeStringViewer = StringTable;
using PreDatafacadeStringViewerView = StringTableView;
} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_STRING_TABLE_HPP
