#include "util/name_table.hpp"
#include "util/simple_logger.hpp"

#include <algorithm>
#include <limits>
#include <fstream>

#include <boost/filesystem/fstream.hpp>

namespace osrm
{
namespace util
{

NameTable::NameTable(const std::string &filename)
{
    boost::filesystem::ifstream name_stream(filename, std::ios::binary);

    name_stream >> m_name_table;

    unsigned number_of_chars = 0;
    name_stream.read(reinterpret_cast<char *>(&number_of_chars), sizeof(number_of_chars));
    BOOST_ASSERT_MSG(0 != number_of_chars, "name file broken");
    m_names_char_list.resize(number_of_chars + 1); //+1 gives sentinel element
    name_stream.read(reinterpret_cast<char *>(&m_names_char_list[0]),
                     number_of_chars * sizeof(m_names_char_list[0]));
    if (0 == m_names_char_list.size())
    {
        util::SimpleLogger().Write(logWARNING) << "list of street names is empty";
    }
}

std::string NameTable::get_name_for_id(const unsigned name_id) const
{
    if (std::numeric_limits<unsigned>::max() == name_id)
    {
        return "";
    }
    auto range = m_name_table.GetRange(name_id);

    std::string result;
    result.reserve(range.size());
    if (range.begin() != range.end())
    {
        result.resize(range.back() - range.front() + 1);
        std::copy(m_names_char_list.begin() + range.front(),
                  m_names_char_list.begin() + range.back() + 1, result.begin());
    }
    return result;
}
} // namespace util
} // namespace osrm
