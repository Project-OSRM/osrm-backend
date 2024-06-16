#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/trip_parameters.hpp"

#include "engine/engine_config.hpp"
#include "util/coordinate.hpp"
#include "util/timing_util.hpp"

#include "osrm/route_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"

#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

#include <boost/assert.hpp>

#include <boost/optional/optional.hpp>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ostream>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace osrm;

namespace
{

class GPSTraces
{
  private:
    std::set<int> trackIDs;
    std::unordered_map<int /* track id */, std::vector<osrm::util::Coordinate>> traces;
    std::vector<osrm::util::Coordinate> coordinates;
    mutable std::mt19937 gen;

    int seed;

  public:
    GPSTraces(int seed) : gen(std::random_device{}()), seed(seed) { gen.seed(seed); }

    void resetSeed() const { gen.seed(seed); }

    bool readCSV(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Error opening file: " << filename << std::endl;
            return false;
        }

        std::string line;
        std::getline(file, line);

        while (std::getline(file, line))
        {
            std::istringstream ss(line);
            std::string token;

            int trackID;
            double latitude, longitude;
            std::string time;

            std::getline(ss, token, ',');
            trackID = std::stoi(token);

            std::getline(ss, token, ',');
            latitude = std::stod(token);

            std::getline(ss, token, ',');
            longitude = std::stod(token);

            // handle empty fields
            if (std::getline(ss, token, ','))
            {
                time = token;
            }

            trackIDs.insert(trackID);
            traces[trackID].emplace_back(osrm::util::Coordinate{
                osrm::util::FloatLongitude{longitude}, osrm::util::FloatLatitude{latitude}});
            coordinates.emplace_back(osrm::util::Coordinate{osrm::util::FloatLongitude{longitude},
                                                            osrm::util::FloatLatitude{latitude}});
        }

        file.close();
        return true;
    }

    const osrm::util::Coordinate &getRandomCoordinate() const
    {
        std::uniform_int_distribution<> dis(0, coordinates.size() - 1);
        return coordinates[dis(gen)];
    }

    const std::vector<osrm::util::Coordinate> &getRandomTrace() const
    {
        std::uniform_int_distribution<> dis(0, trackIDs.size() - 1);
        auto it = trackIDs.begin();
        std::advance(it, dis(gen));
        return traces.at(*it);
    }
};

class Statistics
{
  public:
    void push(double timeMs, int /*iteration*/ = 0)
    {
        times.push_back(timeMs);
        sorted = false;
    }

    double mean() { return sum() / times.size(); }

    double sum()
    {
        double sum = 0;
        for (auto time : times)
        {
            sum += time;
        }
        return sum;
    }

    double min() { return *std::min_element(times.begin(), times.end()); }

    double max() { return *std::max_element(times.begin(), times.end()); }

    double percentile(double p)
    {
        const auto &times = getTimes();
        return times[static_cast<size_t>(p * times.size())];
    }

  private:
    std::vector<double> getTimes()
    {
        if (!sorted)
        {
            std::sort(times.begin(), times.end());
            sorted = true;
        }
        return times;
    }

    std::vector<double> times;

    bool sorted = false;
};

std::ostream &operator<<(std::ostream &os, Statistics &statistics)
{
    os << std::fixed << std::setprecision(2);
    os << "total: " << statistics.sum() << "ms" << std::endl;
    os << "avg: " << statistics.mean() << "ms" << std::endl;
    os << "min: " << statistics.min() << "ms" << std::endl;
    os << "max: " << statistics.max() << "ms" << std::endl;
    os << "p99: " << statistics.percentile(0.99) << "ms" << std::endl;
    return os;
}

void runRouteBenchmark(const OSRM &osrm, const GPSTraces &gpsTraces, int iterations)
{
    struct Benchmark
    {
        std::string name;
        size_t coordinates;
        RouteParameters::OverviewType overview;
        bool steps = false;
        std::optional<size_t> alternatives = std::nullopt;
        std::optional<double> radius = std::nullopt;
    };

    auto run_benchmark = [&](const Benchmark &benchmark)
    {
        Statistics statistics;

        for (int iteration = 0; iteration < iterations; ++iteration)
        {
            gpsTraces.resetSeed();

            auto NUM = 1000;
            for (int i = 0; i < NUM; ++i)
            {
                RouteParameters params;
                params.overview = benchmark.overview;
                params.steps = benchmark.steps;

                for (size_t i = 0; i < benchmark.coordinates; ++i)
                {
                    params.coordinates.push_back(gpsTraces.getRandomCoordinate());
                }

                if (benchmark.alternatives)
                {
                    params.alternatives = *benchmark.alternatives;
                }

                if (benchmark.radius)
                {
                    params.radiuses = std::vector<boost::optional<double>>(
                        params.coordinates.size(), boost::make_optional(*benchmark.radius));
                }

                engine::api::ResultT result = json::Object();
                TIMER_START(routes);
                const auto rc = osrm.Route(params, result);
                TIMER_STOP(routes);

                statistics.push(TIMER_MSEC(routes), iteration);

                auto &json_result = std::get<json::Object>(result);
                if (rc != Status::Ok ||
                    json_result.values.find("routes") == json_result.values.end())
                {
                    auto code = std::get<json::String>(json_result.values["code"]).value;
                    if (code != "NoSegment" && code != "NoRoute")
                    {
                        throw std::runtime_error{"Couldn't route: " + code};
                    }
                }
            }
        }
        std::cout << benchmark.name << std::endl;
        std::cout << statistics << std::endl;
    };

    std::vector<Benchmark> benchmarks = {
        {"10000 routes, 3 coordinates, no alternatives, overview=full, steps=true",
         3,
         RouteParameters::OverviewType::Full,
         true,
         std::nullopt},
        {"10000 routes, 2 coordinates, 3 alternatives, overview=full, steps=true",
         2,
         RouteParameters::OverviewType::Full,
         true,
         3},
        {"10000 routes, 3 coordinates, no alternatives, overview=false, steps=false",
         3,
         RouteParameters::OverviewType::False,
         false,
         std::nullopt},
        {"10000 routes, 2 coordinates, 3 alternatives, overview=false, steps=false",
         2,
         RouteParameters::OverviewType::False,
         false,
         3}};

    for (const auto &benchmark : benchmarks)
    {
        run_benchmark(benchmark);
    }
}

void runMatchBenchmark(const OSRM &osrm, const GPSTraces &gpsTraces)
{
    struct Benchmark
    {
        std::string name;
        std::optional<size_t> radius = std::nullopt;
    };

    auto run_benchmark = [&](const Benchmark &benchmark)
    {
        Statistics statistics;

        auto NUM = 1000;
        for (int i = 0; i < NUM; ++i)
        {
            engine::api::ResultT result = json::Object();

            engine::api::MatchParameters params;
            params.coordinates = gpsTraces.getRandomTrace();
            params.radiuses = {};
            if (benchmark.radius)
            {
                for (size_t index = 0; index < params.coordinates.size(); ++index)
                {
                    params.radiuses.emplace_back(*benchmark.radius);
                }
            }

            TIMER_START(match);
            const auto rc = osrm.Match(params, result);
            TIMER_STOP(match);

            statistics.push(TIMER_MSEC(match));

            auto &json_result = std::get<json::Object>(result);
            if (rc != Status::Ok ||
                json_result.values.find("matchings") == json_result.values.end())
            {
                auto code = std::get<json::String>(json_result.values["code"]).value;
                if (code != "NoSegment" && code != "NoMatch")
                {
                    throw std::runtime_error{"Couldn't route: " + code};
                }
            }
        }

        std::cout << benchmark.name << std::endl;
        std::cout << statistics << std::endl;
    };

    std::vector<Benchmark> benchmarks = {{"1000 matches, default radius"},
                                         {"1000 matches, radius=10", 10},
                                         {"1000 matches, radius=20", 20}};

    for (const auto &benchmark : benchmarks)
    {
        run_benchmark(benchmark);
    }
}

void runNearestBenchmark(const OSRM &osrm, const GPSTraces &gpsTraces)
{
    struct Benchmark
    {
        std::string name;
        std::optional<size_t> number_of_results = std::nullopt;
    };

    auto run_benchmark = [&](const Benchmark &benchmark)
    {
        Statistics statistics;
        auto NUM = 10000;
        for (int i = 0; i < NUM; ++i)
        {
            engine::api::ResultT result = json::Object();
            NearestParameters params;
            params.coordinates.push_back(gpsTraces.getRandomCoordinate());

            if (benchmark.number_of_results)
            {
                params.number_of_results = *benchmark.number_of_results;
            }

            TIMER_START(nearest);
            const auto rc = osrm.Nearest(params, result);
            TIMER_STOP(nearest);

            statistics.push(TIMER_MSEC(nearest));

            auto &json_result = std::get<json::Object>(result);
            if (rc != Status::Ok ||
                json_result.values.find("waypoints") == json_result.values.end())
            {
                auto code = std::get<json::String>(json_result.values["code"]).value;
                if (code != "NoSegment")
                {
                    throw std::runtime_error{"Couldn't find nearest point"};
                }
            }
        }

        std::cout << benchmark.name << std::endl;
        std::cout << statistics << std::endl;
    };

    std::vector<Benchmark> benchmarks = {{"10000 nearest, number_of_results=1", 1},
                                         {"10000 nearest, number_of_results=5", 5},
                                         {"10000 nearest, number_of_results=10", 10}};

    for (const auto &benchmark : benchmarks)
    {
        run_benchmark(benchmark);
    }
}

void runTripBenchmark(const OSRM &osrm, const GPSTraces &gpsTraces)
{
    struct Benchmark
    {
        std::string name;
        size_t coordinates;
    };

    auto run_benchmark = [&](const Benchmark &benchmark)
    {
        Statistics statistics;
        auto NUM = 1000;
        for (int i = 0; i < NUM; ++i)
        {
            engine::api::ResultT result = json::Object();
            TripParameters params;
            params.roundtrip = true;

            for (size_t i = 0; i < benchmark.coordinates; ++i)
            {
                params.coordinates.push_back(gpsTraces.getRandomCoordinate());
            }

            TIMER_START(trip);
            const auto rc = osrm.Trip(params, result);
            TIMER_STOP(trip);

            statistics.push(TIMER_MSEC(trip));

            auto &json_result = std::get<json::Object>(result);
            if (rc != Status::Ok || json_result.values.find("trips") == json_result.values.end())
            {
                auto code = std::get<json::String>(json_result.values["code"]).value;
                if (code != "NoSegment")
                {
                    throw std::runtime_error{"Couldn't find trip"};
                }
            }
        }

        std::cout << benchmark.name << std::endl;
        std::cout << statistics << std::endl;
    };

    std::vector<Benchmark> benchmarks = {
        {"1000 trips, 3 coordinates", 3},
        {"1000 trips, 4 coordinates", 4},
        {"1000 trips, 5 coordinates", 5},
    };

    for (const auto &benchmark : benchmarks)
    {
        run_benchmark(benchmark);
    }
}
void runTableBenchmark(const OSRM &osrm, const GPSTraces &gpsTraces)
{
    struct Benchmark
    {
        std::string name;
        size_t coordinates;
    };

    auto run_benchmark = [&](const Benchmark &benchmark)
    {
        Statistics statistics;
        auto NUM = 250;
        for (int i = 0; i < NUM; ++i)
        {
            engine::api::ResultT result = json::Object();
            TableParameters params;

            for (size_t i = 0; i < benchmark.coordinates; ++i)
            {
                params.coordinates.push_back(gpsTraces.getRandomCoordinate());
            }

            TIMER_START(table);
            const auto rc = osrm.Table(params, result);
            TIMER_STOP(table);

            statistics.push(TIMER_MSEC(table));

            auto &json_result = std::get<json::Object>(result);
            if (rc != Status::Ok ||
                json_result.values.find("durations") == json_result.values.end())
            {
                auto code = std::get<json::String>(json_result.values["code"]).value;
                if (code != "NoSegment")
                {
                    throw std::runtime_error{"Couldn't compute table"};
                }
            }
        }

        std::cout << benchmark.name << std::endl;
        std::cout << statistics << std::endl;
    };

    std::vector<Benchmark> benchmarks = {{"250 tables, 3 coordinates", 3},
                                         {"250 tables, 25 coordinates", 25},
                                         {"250 tables, 50 coordinates", 50}};

    for (const auto &benchmark : benchmarks)
    {
        run_benchmark(benchmark);
    }
}

} // namespace

int main(int argc, const char *argv[])
try
{
    if (argc < 6)
    {
        std::cerr << "Usage: " << argv[0]
                  << " data.osrm <mld|ch> <path to GPS traces.csv> "
                     "<route|match|trip|table|nearest> <number_of_iterations>\n";
        return EXIT_FAILURE;
    }

    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;
    config.storage_config = {argv[1]};
    config.algorithm =
        std::string{argv[2]} == "mld" ? EngineConfig::Algorithm::MLD : EngineConfig::Algorithm::CH;
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    OSRM osrm{config};

    GPSTraces gpsTraces{42};
    gpsTraces.readCSV(argv[3]);

    int iterations = std::stoi(argv[5]);

    const auto benchmarkToRun = std::string{argv[4]};

    if (benchmarkToRun == "route")
    {
        runRouteBenchmark(osrm, gpsTraces, iterations);
    }
    else if (benchmarkToRun == "match")
    {
        runMatchBenchmark(osrm, gpsTraces);
    }
    else if (benchmarkToRun == "nearest")
    {
        runNearestBenchmark(osrm, gpsTraces);
    }
    else if (benchmarkToRun == "trip")
    {
        runTripBenchmark(osrm, gpsTraces);
    }
    else if (benchmarkToRun == "table")
    {
        runTableBenchmark(osrm, gpsTraces);
    }
    else
    {
        std::cerr << "Unknown benchmark: " << benchmarkToRun << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
catch (const std::exception &e)
{
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
