#include "util/name_table.hpp"
#include "storage/io.hpp"
#include "util/log.hpp"

namespace osrm
{
namespace util
{

NameTable::NameTable(const std::string &file_name)
{
    using FileReader = storage::io::FileReader;

    FileReader name_stream_file_reader(file_name, FileReader::HasNoFingerprint);
    const auto file_size = name_stream_file_reader.GetSize();

    m_buffer = BufferType(static_cast<ValueType *>(::operator new(file_size)),
                          [](void *ptr) { ::operator delete(ptr); });
    name_stream_file_reader.ReadInto<char>(m_buffer.get(), file_size);
    m_name_table.reset(m_buffer.get(), m_buffer.get() + file_size);

    if (m_name_table.empty())
    {
        util::Log() << "list of street names is empty in construction of name table from: \""
                    << file_name << "\"";
    }
}

void NameTable::reset(ValueType *begin, ValueType *end)
{
    m_buffer.reset();
    m_name_table.reset(begin, end);
}

StringView NameTable::GetNameForID(const NameID id) const
{
    if (id == INVALID_NAMEID)
        return {};

    return m_name_table.at(id);
}

StringView NameTable::GetDestinationsForID(const NameID id) const
{
    if (id == INVALID_NAMEID)
        return {};

    return m_name_table.at(id + 1);
}

StringView NameTable::GetRefForID(const NameID id) const
{
    if (id == INVALID_NAMEID)
        return {};

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
    return m_name_table.at(id + OFFSET_REF);
}

StringView NameTable::GetPronunciationForID(const NameID id) const
{
    if (id == INVALID_NAMEID)
        return {};

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
    return m_name_table.at(id + OFFSET_PRONUNCIATION);
}

} // namespace util
} // namespace osrm
