#ifndef OSRM_EXTRACTOR_SUFFIX_LIST_HPP_
#define OSRM_EXTRACTOR_SUFFIX_LIST_HPP_

#include <string>
#include <unordered_set>

namespace osrm
{
namespace extractor
{

class ScriptingEnvironment;

// A table containing suffixes.
// At the moment, it is only a front for an unordered set. At some point we might want to make it
// country dependent and have it behave accordingly
class SuffixTable final
{
  public:
    SuffixTable(ScriptingEnvironment &scripting_environment);

    // check whether a string is part of the know suffix list
    bool isSuffix(const std::string &possible_suffix) const;

  private:
    std::unordered_set<std::string> suffix_set;
};

} /* namespace extractor */
} /* namespace osrm */

#endif /* OSRM_EXTRACTOR_SUFFIX_LIST_HPP_ */
