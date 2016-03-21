#ifndef SCRIPTING_ENVIRONMENT_HPP
#define SCRIPTING_ENVIRONMENT_HPP

#include "extractor/profile_properties.hpp"
#include "extractor/raster_source.hpp"

#include <string>
#include <memory>
#include <mutex>
#include <tbb/enumerable_thread_specific.h>

struct lua_State;

namespace osrm
{
namespace extractor
{

/**
 * Creates a lua context and binds osmium way, node and relation objects and
 * ExtractionWay and ExtractionNode to lua objects.
 *
 * Each thread has its own lua state which is implemented with thread specific
 * storage from TBB.
 */
class ScriptingEnvironment
{
  public:
    struct Context
    {
        Context();
        ~Context();

        ProfileProperties properties;
        SourceContainer sources;
        lua_State *state;
    };

    explicit ScriptingEnvironment(const std::string &file_name);

    ScriptingEnvironment(const ScriptingEnvironment &) = delete;
    ScriptingEnvironment &operator=(const ScriptingEnvironment &) = delete;

    Context &GetContex();

  private:
    void InitContext(Context &context);
    std::mutex init_mutex;
    std::string file_name;
    tbb::enumerable_thread_specific<std::shared_ptr<Context>> script_contexts;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_HPP */
