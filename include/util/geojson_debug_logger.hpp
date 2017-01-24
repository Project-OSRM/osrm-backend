#ifndef OSRM_GEOJSON_DEBUG_LOGGER_HPP
#define OSRM_GEOJSON_DEBUG_LOGGER_HPP

#include <fstream>
#include <mutex>
#include <string>

#include "util/exception.hpp"
#include "util/json_container.hpp"
#include "util/json_renderer.hpp"
#include "util/log.hpp"

namespace osrm
{
namespace util
{

// in case we want to do scenario-based logging, we can specify a dedicated logging scenario to be
// able to use multiple files for the same converter
enum class LoggingScenario
{
    eDefault = 0
};

// forward declaration to become friends
template <class geojson_conversion_policy, LoggingScenario scenario> class ScopedGeojsonLoggerGuard;

// a geojson logger requires a conversion policy to transfer arbitrary input into viable geojson.
// features of the same kind are stored in the same geojson file
template <class geojson_conversion_policy, LoggingScenario scenario = LoggingScenario::eDefault>
class GeojsonLogger
{
    // become friends with the guard
    friend class ScopedGeojsonLoggerGuard<geojson_conversion_policy, scenario>;

    // having these private enforces the guard to be used to initialise/close/write
  private:
    // cannot lock, is tooling for locked function
    static void output(bool first, const util::json::Object &object)
    {
        if (!first)
            ofs << ",\n\t\t";
        // objects are simply forwarded
        util::json::render(ofs, object);
    }

    // cannot lock, is tooling for locked function
    static void output(bool first, const util::json::Array &array)
    {
        for (const auto &object : array.values)
        {
            if (!first)
                ofs << ",\n\t\t";

            util::json::render(ofs, object.get<util::json::Object>());

            first = false;
        }
    }

    // writes a single feature into the Geojson file
    template <typename... Args> static bool Write(Args &&... args)
    {
        // make sure to syncronize logging output, our writing should be sequential
        std::lock_guard<std::mutex> guard(lock);

        // if there is no logfile, we cannot write (possible reason: the guard might be out of scope
        // (e.g. if it is anonymous))
        if (!ofs.is_open() || (nullptr == policy))
        {
            throw util::exception("Trying to use the geojson printer without an open logger.");
        }

        // use our policy to convert the arguments into geojson, this can be done in parallel
        const auto json_object = (*policy)(std::forward<Args>(args)...);

        // different features are separated by `,`
        // since we are not building a full json collection of features, we have to do some of the
        // bookeeping ourselves. This prevens us from doing one huge json object, if we are
        // processing larger results
        output(first, json_object);
        first = false;

        return static_cast<bool>(ofs);
    }

    // Opening a logger, we initialize a geojson feature collection
    // to be later filled with additional data
    static bool Open(const std::string &logfile)
    {
        // if a file is open, close it off. When this function returns, there should be an open
        // logfile. However, there is a brief period between closing and openen where we could miss
        // out on log output. Such a sad life
        if (ofs.is_open())
        {
            util::Log(logWARNING)
                << "Overwriting " << logfile
                << ". Is this desired behaviour? If this message occurs more than once rethink the "
                   "location of your Logger Guard.";
            Close();
        }

        // make sure to syncronize logging output, cannot be locked earlier, since Close also locks
        // and we don't want deadlocks
        std::lock_guard<std::mutex> guard(lock);
        ofs.open(logfile, std::ios::binary);

        // set up a feature collection
        ofs << "{\n\t\"type\": \"FeatureCollection\",\n\t\"features\": [\n\t";
        // remember whether we need to output a colon
        first = true;

        return static_cast<bool>(ofs);
    }

    // finalising touches on the GeoJson
    static bool Close()
    {
        // make sure to syncronize logging output
        std::lock_guard<std::mutex> guard(lock);

        // finishe the geojson feature collection and close it all off
        if (ofs.is_open())
        {
            ofs << "\n\t]\n}";
            ofs.close();
        }

        return static_cast<bool>(ofs);
    }

    static void SetPolicy(geojson_conversion_policy *new_policy) { policy = new_policy; }

  private:
    static bool first;
    static std::mutex lock;
    static std::ofstream ofs;

    static geojson_conversion_policy *policy;
};

// make sure to do opening and closing of our guard
template <class geojson_conversion_policy, LoggingScenario scenario = LoggingScenario::eDefault>
class ScopedGeojsonLoggerGuard
{
  public:
    template <typename... Args>
    ScopedGeojsonLoggerGuard(const std::string &logfile, Args &&... args)
        : policy(std::forward<Args>(args)...)
    {
        GeojsonLogger<geojson_conversion_policy, scenario>::Open(logfile);
        GeojsonLogger<geojson_conversion_policy, scenario>::SetPolicy(&policy);
    }

    ~ScopedGeojsonLoggerGuard()
    {
        GeojsonLogger<geojson_conversion_policy, scenario>::Close();
        GeojsonLogger<geojson_conversion_policy, scenario>::SetPolicy(nullptr);
    }

    template <typename... Args> static bool Write(Args &&... args)
    {
        return GeojsonLogger<geojson_conversion_policy, scenario>::Write(
            std::forward<Args>(args)...);
    }

  private:
    geojson_conversion_policy policy;
};

template <class geojson_conversion_policy, LoggingScenario scenario>
bool GeojsonLogger<geojson_conversion_policy, scenario>::first;

template <class geojson_conversion_policy, LoggingScenario scenario>
std::mutex GeojsonLogger<geojson_conversion_policy, scenario>::lock;

template <class geojson_conversion_policy, LoggingScenario scenario>
std::ofstream GeojsonLogger<geojson_conversion_policy, scenario>::ofs;

template <class geojson_conversion_policy, LoggingScenario scenario>
geojson_conversion_policy *GeojsonLogger<geojson_conversion_policy, scenario>::policy;

} // namespace util
} // namespace osrm

#endif /* OSRM_GEOJSON_DEBUG_LOGGER_HPP */
