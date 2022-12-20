#ifndef OSRM_EXTRACTOR_SUFFIX_LIST_HPP_
#define OSRM_EXTRACTOR_SUFFIX_LIST_HPP_

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace osrm::extractor
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
    bool isSuffix(std::string_view possible_suffix) const;

  private:
    // Store lower-cased strings in SuffixTable and a set of StringViews for quick membership
    // checks.
    //
    // Why not uset<StringView>? StringView is non-owning, the vector<string> holds the string
    // contents, the set<StringView> only holds [first,last) pointers into the vector.
    //
    // Then why not simply uset<string>? Because membership tests against StringView keys would
    // require us to first convert StringViews into strings (allocation), do the membership,
    // and destroy the StringView again.
    std::vector<std::string> suffixes;
    std::unordered_set<std::string_view> suffix_set;
};

} // namespace osrm::extractor

#endif /* OSRM_EXTRACTOR_SUFFIX_LIST_HPP_ */
