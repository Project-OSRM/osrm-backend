
#include "osrm/json_container.hpp"
#include "util/json_container.hpp"
#include "util/json_renderer.hpp"
#include "util/timing_util.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>

int main(int, char **)
{
    using namespace osrm;

    const auto location = json::Array{{{7.437070}, {43.749248}}};

    json::Object reference{
        {{"code", "Ok"},
         {"waypoints",
          json::Array{{json::Object{{{"name", "Boulevard du Larvotto"},
                                     {"location", location},
                                     {"distance", round(0.137249 * 1000000)},
                                     {"hint", ""}}},
                       json::Object{{{"name", "Boulevard du Larvotto"},
                                     {"location", location},
                                     {"distance", round(0.137249 * 1000000)},
                                     {"hint", ""}}}}}},
         {"routes",
          json::Array{{json::Object{
              {{"distance", 0.},
               {"duration", 0.},
               {"weight", 0.},
               {"weight_name", "routability"},
               {"geometry", "yw_jGupkl@??"},
               {"legs",
                json::Array{{json::Object{
                    {{"distance", 0.},
                     {"duration", 0.},
                     {"weight", 0.},
                     {"summary", "Boulevard du Larvotto"},
                     {"steps",
                      json::Array{{{json::Object{{{"duration", 0.},
                                                  {"distance", 0.},
                                                  {"weight", 0.},
                                                  {"geometry", "yw_jGupkl@??"},
                                                  {"name", "Boulevard du Larvotto"},
                                                  {"mode", "driving"},
                                                  {"driving_side", "right"},
                                                  {"maneuver",
                                                   json::Object{{
                                                       {"location", location},
                                                       {"bearing_before", 0},
                                                       {"bearing_after", 238},
                                                       {"type", "depart"},
                                                   }}},
                                                  {"intersections",
                                                   json::Array{{json::Object{
                                                       {{"location", location},
                                                        {"bearings", json::Array{{238}}},
                                                        {"entry", json::Array{{json::True()}}},
                                                        {"out", 0}}}}}}}}},

                                   json::Object{{{"duration", 0.},
                                                 {"distance", 0.},
                                                 {"weight", 0.},
                                                 {"geometry", "yw_jGupkl@"},
                                                 {"name", "Boulevard du Larvotto"},
                                                 {"mode", "driving"},
                                                 {"driving_side", "right"},
                                                 {"maneuver",
                                                  json::Object{{{"location", location},
                                                                {"bearing_before", 238},
                                                                {"bearing_after", 0},
                                                                {"type", "arrive"}}}},
                                                 {"intersections",
                                                  json::Array{{json::Object{
                                                      {{"location", location},
                                                       {"bearings", json::Array{{58}}},
                                                       {"entry", json::Array{{json::True()}}},
                                                       {"in", 0}}}}}}

                                   }}}}}}}}}}}}}}}}};
    json::Array arr;
    for (size_t index = 0; index < 4096; ++index)
    {
        arr.values.push_back(reference);
    }
    json::Object obj{{{"arr", arr}}};

    TIMER_START(string);
    std::string out_str;
    json::render(out_str, obj);
    TIMER_STOP(string);
    std::cout << "String: " << TIMER_MSEC(string) << "ms" << std::endl;

    TIMER_START(stringstream);
    std::stringstream ss;
    json::render(ss, obj);
    std::string out_ss_str{ss.str()};
    TIMER_STOP(stringstream);

    std::cout << "Stringstream: " << TIMER_MSEC(stringstream) << "ms" << std::endl;
    TIMER_START(vector);
    std::vector<char> out_vec;
    json::render(out_vec, obj);
    TIMER_STOP(vector);
    std::cout << "Vector: " << TIMER_MSEC(vector) << "ms" << std::endl;

    return EXIT_SUCCESS;
}
