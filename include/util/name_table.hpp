#ifndef OSRM_UTIL_NAME_TABLE_HPP
#define OSRM_UTIL_NAME_TABLE_HPP

#include "util/range_table.hpp"
#include "util/shared_memory_vector_wrapper.hpp"

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
  private:
    // FIXME should this use shared memory
    RangeTable<16, false> m_name_table;
    ShM<char, false>::vector m_names_char_list;

  public:
    NameTable(const std::string &filename);
    std::string GetNameForID(const unsigned name_id) const;
};
} // namespace util
} // namespace osrm

#endif // OSRM_UTIL_NAME_TABLE_HPP
