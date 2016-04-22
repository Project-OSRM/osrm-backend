#include "extractor/suffix_table.hpp"

#include "util/lua_util.hpp"
#include "util/simple_logger.hpp"

#include <boost/assert.hpp>
#include <boost/ref.hpp>

#include <vector>

namespace osrm
{
namespace extractor
{

void SuffixTable::fill(lua_State *lua_state)
{
    BOOST_ASSERT(lua_state != nullptr);
    if (!util::luaFunctionExists(lua_state, "get_name_suffix_list"))
        return;

    std::vector<std::string> suffixes_vector;
    try
    {
        // call lua profile to compute turn penalty
        luabind::call_function<void>(lua_state, "get_name_suffix_list",
                                     boost::ref(suffixes_vector));
    }
    catch (const luabind::error &er)
    {
        util::SimpleLogger().Write(logWARNING) << er.what();
    }

    for (const auto &suffix : suffixes_vector)
        suffix_set.insert(suffix);
}

bool SuffixTable::isSuffix(const std::string &possible_suffix) const
{
    return suffix_set.count(possible_suffix) > 0;
}

} /* namespace extractor */
} /* namespace osrm */
