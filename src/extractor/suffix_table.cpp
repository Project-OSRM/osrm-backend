#include "extractor/suffix_table.hpp"

#include "extractor/scripting_environment.hpp"

#include <boost/algorithm/string.hpp>

namespace osrm
{
namespace extractor
{

SuffixTable::SuffixTable(ScriptingEnvironment &scripting_environment)
{
    std::vector<std::string> suffixes_vector = scripting_environment.GetNameSuffixList();
    for (auto &suffix : suffixes_vector)
        boost::algorithm::to_lower(suffix);
    suffix_set.insert(std::begin(suffixes_vector), std::end(suffixes_vector));
}

bool SuffixTable::isSuffix(const std::string &possible_suffix) const
{
    return suffix_set.count(possible_suffix) > 0;
}

} /* namespace extractor */
} /* namespace osrm */
