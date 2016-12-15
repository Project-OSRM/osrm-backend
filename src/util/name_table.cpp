#include "util/name_table.hpp"
#include "storage/io.hpp"
#include "util/exception.hpp"
#include "util/log.hpp"

#include <algorithm>
#include <iterator>
#include <limits>

#include <boost/filesystem/fstream.hpp>

namespace osrm
{
namespace util
{

NameTable::NameTable(const std::string &filename)
{
    storage::io::FileReader name_stream_file_reader(filename,
                                                    storage::io::FileReader::HasNoFingerprint);

    m_name_table.ReadARangeTable(name_stream_file_reader);

    const auto number_of_chars = name_stream_file_reader.ReadElementCount32();

    m_names_char_list.resize(number_of_chars + 1); //+1 gives sentinel element
    m_names_char_list.back() = 0;
    if (number_of_chars > 0)
    {
        name_stream_file_reader.ReadInto(&m_names_char_list[0], number_of_chars);
    }
    else
    {
        util::Log() << "list of street names is empty in construction of name table from: \""
                    << filename << "\"";
    }
}

StringView NameTable::GetNameForID(const NameID id) const
{
    if (std::numeric_limits<NameID>::max() == id)
    {
        return {};
    }

    auto range = m_name_table.GetRange(id);

    if (range.begin() == range.end())
    {
        return {};
    }

    auto first = begin(m_names_char_list) + range.front();
    auto last = begin(m_names_char_list) + range.back() + 1;
    const std::size_t len = last - first;

    return StringView{&*first, len};
}

StringView NameTable::GetRefForID(const NameID id) const
{
    // Way string data is stored in blocks based on `id` as follows:
    //
    // | name | destination | pronunciation | ref |
    //                                      ^     ^
    //                                      [range)
    //                                       ^ id + 3
    //
    // `id + offset` gives us the range of chars.
    //
    // Offset 0 is name, 1 is destination, 2 is pronunciation, 3 is ref.
    // See datafacades and extractor callbacks for details.
    const constexpr auto OFFSET_REF = 3u;
    return GetNameForID(id + OFFSET_REF);
}

StringView NameTable::GetPronunciationForID(const NameID id) const
{
    // Way string data is stored in blocks based on `id` as follows:
    //
    // | name | destination | pronunciation | ref |
    //                      ^               ^
    //                      [range)
    //                       ^ id + 2
    //
    // `id + offset` gives us the range of chars.
    //
    // Offset 0 is name, 1 is destination, 2 is pronunciation, 3 is ref.
    // See datafacades and extractor callbacks for details.
    const constexpr auto OFFSET_PRONUNCIATION = 2u;
    return GetNameForID(id + OFFSET_PRONUNCIATION);
}

} // namespace util
} // namespace osrm
