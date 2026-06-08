#include "extractor/suffix_table.hpp"
#include "extractor/scripting_environment.hpp"

#include <algorithm>
#include <iterator>

#include <boost/algorithm/string.hpp>

namespace osrm::extractor
{

SuffixTable::SuffixTable(ScriptingEnvironment &scripting_environment)
{
    suffixes = scripting_environment.GetNameSuffixList();

    for (auto &suffix : suffixes)
        boost::algorithm::to_lower(suffix);

    auto into = std::inserter(suffix_set, end(suffix_set));
    auto to_view = [](const auto &s) { return std::string_view{s}; };
    std::transform(begin(suffixes), end(suffixes), into, to_view);
}

bool SuffixTable::isSuffix(const std::string &possible_suffix) const
{
    return isSuffix(std::string_view{possible_suffix});
}

bool SuffixTable::isSuffix(std::string_view possible_suffix) const
{
    return suffix_set.contains(possible_suffix);
}

} // namespace osrm::extractor
