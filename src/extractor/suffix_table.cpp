#include "extractor/suffix_table.hpp"
#include "extractor/scripting_environment.hpp"

#include <algorithm>
#include <iterator>

#include <boost/algorithm/string.hpp>

namespace osrm
{
namespace extractor
{

SuffixTable::SuffixTable(ScriptingEnvironment &scripting_environment)
{
    suffixes = scripting_environment.GetNameSuffixList();

    for (auto &suffix : suffixes)
        boost::algorithm::to_lower(suffix);

    auto into = std::inserter(suffix_set, end(suffix_set));
    auto to_view = [](const auto &s) { return util::StringView{s}; };
    std::transform(begin(suffixes), end(suffixes), into, to_view);
}

bool SuffixTable::isSuffix(const std::string &possible_suffix) const
{
    return isSuffix(util::StringView{possible_suffix});
}

bool SuffixTable::isSuffix(util::StringView possible_suffix) const
{
    return suffix_set.count(possible_suffix) > 0;
}

} /* namespace extractor */
} /* namespace osrm */
