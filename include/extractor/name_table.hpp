#ifndef OSRM_EXTRACTOR_NAME_TABLE_HPP
#define OSRM_EXTRACTOR_NAME_TABLE_HPP

#include "util/indexed_data.hpp"
#include "util/string_view.hpp"
#include "util/typedefs.hpp"

#include <string>

namespace osrm
{
namespace extractor
{

namespace detail
{
template <storage::Ownership Ownership> class NameTableImpl;
}

namespace serialization
{
template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::NameTableImpl<Ownership> &index_data);

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::NameTableImpl<Ownership> &index_data);
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
template <storage::Ownership Ownership> class NameTableImpl
{
  public:
    using IndexedData =
        util::detail::IndexedDataImpl<util::VariableGroupBlock<16, util::StringView>, Ownership>;
    using ResultType = typename IndexedData::ResultType;
    using ValueType = typename IndexedData::ValueType;

    NameTableImpl() {}

    NameTableImpl(IndexedData indexed_data_) : indexed_data{std::move(indexed_data_)} {}

    util::StringView GetNameForID(const NameID id) const
    {
        if (id == INVALID_NAMEID)
            return {};

        return indexed_data.at(id + 0);
    }

    util::StringView GetDestinationsForID(const NameID id) const
    {
        if (id == INVALID_NAMEID)
            return {};

        return indexed_data.at(id + 1);
    }

    util::StringView GetExitsForID(const NameID id) const
    {
        if (id == INVALID_NAMEID)
            return {};

        return indexed_data.at(id + 4);
    }

    util::StringView GetRefForID(const NameID id) const
    {
        if (id == INVALID_NAMEID)
            return {};

        const constexpr auto OFFSET_REF = 3u;
        return indexed_data.at(id + OFFSET_REF);
    }

    util::StringView GetPronunciationForID(const NameID id) const
    {
        if (id == INVALID_NAMEID)
            return {};

        const constexpr auto OFFSET_PRONUNCIATION = 2u;
        return indexed_data.at(id + OFFSET_PRONUNCIATION);
    }

    friend void serialization::read<Ownership>(storage::tar::FileReader &reader,
                                               const std::string &name,
                                               NameTableImpl &index_data);

    friend void serialization::write<Ownership>(storage::tar::FileWriter &writer,
                                                const std::string &name,
                                                const NameTableImpl &index_data);

  private:
    IndexedData indexed_data;
};
} // namespace detail

using NameTable = detail::NameTableImpl<storage::Ownership::Container>;
using NameTableView = detail::NameTableImpl<storage::Ownership::View>;
} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_NAME_TABLE_HPP
