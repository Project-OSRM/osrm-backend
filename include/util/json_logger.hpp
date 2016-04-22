#ifndef JSON_LOGGER_HPP
#define JSON_LOGGER_HPP

#include "osrm/json_container.hpp"

#include <boost/thread/tss.hpp>

#include <string>
#include <unordered_map>

namespace osrm
{
namespace util
{
namespace json
{

// Used to append additional debugging information to the JSON response in a
// thread safe manner.
class Logger
{
    using MapT = std::unordered_map<std::string, Value>;

  public:
    static Logger *get()
    {
        static Logger logger;

        bool return_logger = true;
#ifdef NDEBUG
        return_logger = false;
#endif
#ifdef ENABLE_JSON_LOGGING
        return_logger = true;
#endif

        if (return_logger)
        {
            return &logger;
        }

        return nullptr;
    }

    void initialize(const std::string &name)
    {
        if (!map.get())
        {
            map.reset(new MapT());
        }
        (*map)[name] = Object();
    }

    void render(const std::string &name, Object &obj) const { obj.values["debug"] = map->at(name); }

    boost::thread_specific_ptr<MapT> map;
};
}
}
}

#endif /* JSON_LOGGER_HPP */
