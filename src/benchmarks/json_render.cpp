
#include "osrm/json_container.hpp"
#include "util/json_container.hpp"
#include "util/json_renderer.hpp"
#include "util/timing_util.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>

// #ifdef _WIN32
// #pragma optimize("", off)
// template <class T> void dont_optimize_away(T &&datum) { T local = datum; }
// #pragma optimize("", on)
// #else
// template <class T> void dont_optimize_away(T &&datum) { asm volatile("" : "+r"(datum)); }
// #endif

// template <std::size_t num_rounds, std::size_t num_entries, typename VectorT>
// auto measure_random_access()
// {
//     std::vector<std::size_t> indices(num_entries);
//     std::iota(indices.begin(), indices.end(), 0);
//     std::mt19937 g(1337);
//     std::shuffle(indices.begin(), indices.end(), g);

//     VectorT vector(num_entries);

//     TIMER_START(write);
//     for (auto round : util::irange<std::size_t>(0, num_rounds))
//     {
//         for (auto idx : util::irange<std::size_t>(0, num_entries))
//         {
//             vector[indices[idx]] = idx + round;
//         }
//     }
//     TIMER_STOP(write);

//     TIMER_START(read);
//     auto sum = 0;
//     for (auto round : util::irange<std::size_t>(0, num_rounds))
//     {
//         sum = round;
//         for (auto idx : util::irange<std::size_t>(0, num_entries))
//         {
//             sum += vector[indices[idx]];
//         }
//         dont_optimize_away(sum);
//     }
//     TIMER_STOP(read);

//     return Measurement{TIMER_MSEC(write), TIMER_MSEC(read)};
// }

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

    TIMER_START(stringstream);
    std::stringstream ss;
    json::render(ss, obj);
    std::string s{ss.str()};
    TIMER_STOP(stringstream);
    std::cout << "String: " << TIMER_MSEC(stringstream) << "ms" << std::endl;
    TIMER_START(vector);
    std::vector<char> vec;
    json::render(vec, obj);
    TIMER_STOP(vector);
    std::cout << "Vector: " << TIMER_MSEC(vector) << "ms" << std::endl;
    // (void)s;
    // std::cerr << ss << "\n";
    return EXIT_SUCCESS;
}
