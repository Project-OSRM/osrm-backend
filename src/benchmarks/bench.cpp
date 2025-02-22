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

#include "util/meminfo.hpp"
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

    std::vector<osrm::util::Coordinate> getRandomTrace() const
    {
        std::uniform_int_distribution<> dis(0, trackIDs.size() - 1);
        auto it = trackIDs.begin();
        std::advance(it, dis(gen));

        const auto &trace = traces.at(*it);

        std::uniform_int_distribution<> length_dis(50, 100);
        size_t length = length_dis(gen);
        if (trace.size() <= length + 1)
        {
            return trace;
        }

        std::uniform_int_distribution<> start_dis(0, trace.size() - length - 1);
        size_t start_index = start_dis(gen);

        return std::vector<osrm::util::Coordinate>(trace.begin() + start_index,
                                                   trace.begin() + start_index + length);
    }
};

// Struct to hold confidence interval data
struct ConfidenceInterval
{
    double mean;
    double confidence;
    double min;
    double max;
};

// Helper function to calculate the bootstrap confidence interval
ConfidenceInterval confidenceInterval(const std::vector<double> &data,
                                      int num_samples = 1000,
                                      double confidence_level = 0.95)
{
    std::vector<double> means;
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, data.size() - 1);

    for (int i = 0; i < num_samples; ++i)
    {
        std::vector<double> sample;
        for (size_t j = 0; j < data.size(); ++j)
        {
            sample.push_back(data[distribution(generator)]);
        }
        double sample_mean = std::accumulate(sample.begin(), sample.end(), 0.0) / sample.size();
        means.push_back(sample_mean);
    }

    std::sort(means.begin(), means.end());
    double lower_bound = means[(int)((1 - confidence_level) / 2 * num_samples)];
    double upper_bound = means[(int)((1 + confidence_level) / 2 * num_samples)];
    double mean = std::accumulate(means.begin(), means.end(), 0.0) / means.size();

    ConfidenceInterval ci = {mean,
                             (upper_bound - lower_bound) / 2,
                             *std::min_element(data.begin(), data.end()),
                             *std::max_element(data.begin(), data.end())};
    return ci;
}

class Statistics
{
  public:
    explicit Statistics(int iterations) : times(iterations) {}

    void push(double timeMs, int iteration) { times[iteration].push_back(timeMs); }

    ConfidenceInterval mean()
    {
        std::vector<double> means;
        means.reserve(times.size());
        for (const auto &iter_times : times)
        {
            means.push_back(std::accumulate(iter_times.begin(), iter_times.end(), 0.0) /
                            iter_times.size());
        }
        return confidenceInterval(means);
    }

    ConfidenceInterval total()
    {
        std::vector<double> sums;
        sums.reserve(times.size());
        for (const auto &iter_times : times)
        {
            sums.push_back(std::accumulate(iter_times.begin(), iter_times.end(), 0.0));
        }
        return confidenceInterval(sums);
    }

    ConfidenceInterval min()
    {
        std::vector<double> mins;
        mins.reserve(times.size());
        for (const auto &iter_times : times)
        {
            mins.push_back(*std::min_element(iter_times.begin(), iter_times.end()));
        }
        return confidenceInterval(mins);
    }

    ConfidenceInterval max()
    {
        std::vector<double> maxs;
        maxs.reserve(times.size());
        for (const auto &iter_times : times)
        {
            maxs.push_back(*std::max_element(iter_times.begin(), iter_times.end()));
        }
        return confidenceInterval(maxs);
    }

    ConfidenceInterval percentile(double p)
    {
        std::vector<double> percentiles;
        percentiles.reserve(times.size());
        for (const auto &iter_times : times)
        {
            auto sorted_times = iter_times;
            std::sort(sorted_times.begin(), sorted_times.end());
            percentiles.push_back(sorted_times[static_cast<size_t>(p * sorted_times.size())]);
        }
        return confidenceInterval(percentiles);
    }

    ConfidenceInterval ops_per_sec()
    {
        std::vector<double> ops;
        ops.reserve(times.size());
        for (const auto &iter_times : times)
        {
            double total_time = std::accumulate(iter_times.begin(), iter_times.end(), 0.0) / 1000.0;
            ops.push_back(iter_times.size() / total_time);
        }
        return confidenceInterval(ops);
    }

  private:
    // vector of times for each iteration
    std::vector<std::vector<double>> times;
};

std::ostream &operator<<(std::ostream &os, Statistics &statistics)
{
    os << std::fixed << std::setprecision(2);

    ConfidenceInterval mean_ci = statistics.mean();
    ConfidenceInterval total_ci = statistics.total();
    ConfidenceInterval min_ci = statistics.min();
    ConfidenceInterval max_ci = statistics.max();
    ConfidenceInterval p99_ci = statistics.percentile(0.99);
    ConfidenceInterval ops_ci = statistics.ops_per_sec();

    os << "ops: " << ops_ci.mean << " ± " << ops_ci.confidence << " ops/s. "
       << "best: " << ops_ci.max << "ops/s." << std::endl;
    os << "total: " << total_ci.mean << " ± " << total_ci.confidence << "ms. "
       << "best: " << total_ci.min << "ms." << std::endl;
    os << "avg: " << mean_ci.mean << " ± " << mean_ci.confidence << "ms" << std::endl;
    os << "min: " << min_ci.mean << " ± " << min_ci.confidence << "ms" << std::endl;
    os << "max: " << max_ci.mean << " ± " << max_ci.confidence << "ms" << std::endl;
    os << "p99: " << p99_ci.mean << " ± " << p99_ci.confidence << "ms" << std::endl;

    return os;
}

template <typename Benchmark, typename BenchmarkBody>
void runBenchmarks(const std::vector<Benchmark> &benchmarks,
                   int iterations,
                   int opsPerIteration,
                   const OSRM &osrm,
                   const GPSTraces &gpsTraces,
                   const BenchmarkBody &benchmarkBody)
{
    for (const auto &benchmark : benchmarks)
    {
        Statistics statistics{iterations};
        for (int iteration = 0; iteration < iterations; ++iteration)
        {
            gpsTraces.resetSeed();

            for (int i = 0; i < opsPerIteration; ++i)
            {
                benchmarkBody(iteration, benchmark, osrm, gpsTraces, statistics);
            }
        }
        std::cout << benchmark.name << std::endl;
        std::cout << statistics << std::endl;
    }
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
    std::vector<Benchmark> benchmarks = {
        {"1000 routes, 3 coordinates, no alternatives, overview=full, steps=true",
         3,
         RouteParameters::OverviewType::Full,
         true,
         std::nullopt},
        {"1000 routes, 2 coordinates, 3 alternatives, overview=full, steps=true",
         2,
         RouteParameters::OverviewType::Full,
         true,
         3},
        {"1000 routes, 3 coordinates, no alternatives, overview=false, steps=false",
         3,
         RouteParameters::OverviewType::False,
         false,
         std::nullopt},
        {"1000 routes, 2 coordinates, 3 alternatives, overview=false, steps=false",
         2,
         RouteParameters::OverviewType::False,
         false,
         3}};

    runBenchmarks(benchmarks,
                  iterations,
                  1000,
                  osrm,
                  gpsTraces,
                  [](int iteration,
                     const Benchmark &benchmark,
                     const OSRM &osrm,
                     const GPSTraces &gpsTraces,
                     Statistics &statistics)
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
                          params.radiuses = std::vector<std::optional<double>>(
                              params.coordinates.size(), std::make_optional(*benchmark.radius));
                      }

                      engine::api::ResultT result = json::Object();
                      TIMER_START(routes);
                      const auto rc = osrm.Route(params, result);
                      TIMER_STOP(routes);

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
                      else
                      {

                          statistics.push(TIMER_MSEC(routes), iteration);
                      }
                  });
}

void runMatchBenchmark(const OSRM &osrm, const GPSTraces &gpsTraces, int iterations)
{
    struct Benchmark
    {
        std::string name;
        std::optional<size_t> radius = std::nullopt;
    };

    std::vector<Benchmark> benchmarks = {{"500 matches, default radius"},
                                         {"500 matches, radius=10", 10},
                                         {"500 matches, radius=20", 20}};

    runBenchmarks(benchmarks,
                  iterations,
                  500,
                  osrm,
                  gpsTraces,
                  [](int iteration,
                     const Benchmark &benchmark,
                     const OSRM &osrm,
                     const GPSTraces &gpsTraces,
                     Statistics &statistics)
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
                      else
                      {
                          statistics.push(TIMER_MSEC(match), iteration);
                      }
                  });
}

void runNearestBenchmark(const OSRM &osrm, const GPSTraces &gpsTraces, int iterations)
{
    struct Benchmark
    {
        std::string name;
        std::optional<size_t> number_of_results = std::nullopt;
    };

    std::vector<Benchmark> benchmarks = {{"10000 nearest, number_of_results=1", 1},
                                         {"10000 nearest, number_of_results=5", 5},
                                         {"10000 nearest, number_of_results=10", 10}};

    runBenchmarks(benchmarks,
                  iterations,
                  10000,
                  osrm,
                  gpsTraces,
                  [](int iteration,
                     const Benchmark &benchmark,
                     const OSRM &osrm,
                     const GPSTraces &gpsTraces,
                     Statistics &statistics)
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
                      else
                      {
                          statistics.push(TIMER_MSEC(nearest), iteration);
                      }
                  });
}

void runTripBenchmark(const OSRM &osrm, const GPSTraces &gpsTraces, int iterations)
{
    struct Benchmark
    {
        std::string name;
        size_t coordinates;
    };

    std::vector<Benchmark> benchmarks = {
        {"250 trips, 3 coordinates", 3},
        {"250 trips, 5 coordinates", 5},
    };

    runBenchmarks(benchmarks,
                  iterations,
                  250,
                  osrm,
                  gpsTraces,
                  [](int iteration,
                     const Benchmark &benchmark,
                     const OSRM &osrm,
                     const GPSTraces &gpsTraces,
                     Statistics &statistics)
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

                      auto &json_result = std::get<json::Object>(result);
                      if (rc != Status::Ok ||
                          json_result.values.find("trips") == json_result.values.end())
                      {
                          auto code = std::get<json::String>(json_result.values["code"]).value;
                          if (code != "NoSegment")
                          {
                              throw std::runtime_error{"Couldn't find trip"};
                          }
                      }
                      else
                      {
                          statistics.push(TIMER_MSEC(trip), iteration);
                      }
                  });
}
void runTableBenchmark(const OSRM &osrm, const GPSTraces &gpsTraces, int iterations)
{
    struct Benchmark
    {
        std::string name;
        size_t coordinates;
    };

    std::vector<Benchmark> benchmarks = {{"250 tables, 3 coordinates", 3},
                                         {"250 tables, 25 coordinates", 25},
                                         {"250 tables, 50 coordinates", 50}};

    runBenchmarks(benchmarks,
                  iterations,
                  250,
                  osrm,
                  gpsTraces,
                  [](int iteration,
                     const Benchmark &benchmark,
                     const OSRM &osrm,
                     const GPSTraces &gpsTraces,
                     Statistics &statistics)
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

                      statistics.push(TIMER_MSEC(table), iteration);

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
                  });
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
        runMatchBenchmark(osrm, gpsTraces, iterations);
    }
    else if (benchmarkToRun == "nearest")
    {
        runNearestBenchmark(osrm, gpsTraces, iterations);
    }
    else if (benchmarkToRun == "trip")
    {
        runTripBenchmark(osrm, gpsTraces, iterations);
    }
    else if (benchmarkToRun == "table")
    {
        runTableBenchmark(osrm, gpsTraces, iterations);
    }
    else
    {
        std::cerr << "Unknown benchmark: " << benchmarkToRun << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Peak RAM: " << std::setprecision(3)
              << static_cast<double>(osrm::util::PeakRAMUsedInBytes()) /
                     static_cast<double>((1024 * 1024))
              << "MB" << std::endl;

    return EXIT_SUCCESS;
}
catch (const std::exception &e)
{
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
