#include "extractor/suffix_table.hpp"

#include "extractor/scripting_environment.hpp"

namespace osrm
{
namespace extractor
{

SuffixTable::SuffixTable(ScriptingContext &scripting_context)
    : suffix_set(scripting_context.getNameSuffixList())
{
}

bool SuffixTable::isSuffix(const std::string &possible_suffix) const
{
    return suffix_set.count(possible_suffix) > 0;
}

} /* namespace extractor */
} /* namespace osrm */
