#include "util/name_table.hpp"
#include "util/exception.hpp"
#include "util/simple_logger.hpp"

#include <algorithm>
#include <fstream>
#include <limits>

#include <boost/filesystem/fstream.hpp>

namespace osrm
{
namespace util
{

NameTable::NameTable(const std::string &filename)
{
    boost::filesystem::ifstream name_stream(filename, std::ios::binary);

    if (!name_stream)
        throw exception("Failed to open " + filename + " for reading.");

    name_stream >> m_name_table;

    unsigned number_of_chars = 0;
    name_stream.read(reinterpret_cast<char *>(&number_of_chars), sizeof(number_of_chars));
    if (!name_stream)
        throw exception("Encountered invalid file, failed to read number of contained chars");

    m_names_char_list.resize(number_of_chars + 1); //+1 gives sentinel element
    m_names_char_list.back() = 0;
    if (number_of_chars > 0)
    {
        name_stream.read(reinterpret_cast<char *>(&m_names_char_list[0]),
                         number_of_chars * sizeof(m_names_char_list[0]));
    }
    else
    {
        util::SimpleLogger().Write(logINFO)
            << "list of street names is empty in construction of name table from: \"" << filename
            << "\"";
    }
    if (!name_stream)
        throw exception("Failed to read " + std::to_string(number_of_chars) + " characters from " +
                        filename);
}

std::string NameTable::GetNameForID(const unsigned name_id) const
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
                  m_names_char_list.begin() + range.back() + 1,
                  result.begin());
    }
    return result;
}
} // namespace util
} // namespace osrm
