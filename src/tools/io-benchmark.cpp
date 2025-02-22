#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/log.hpp"
#include "util/timing_util.hpp"

#include <cmath>
#include <cstdio>
#include <fcntl.h>
#ifdef __linux__
#include <malloc.h>
#endif

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <random>
#include <vector>

namespace osrm::tools
{

const unsigned NUMBER_OF_ELEMENTS = 268435456;

struct Statistics
{
    double min, max, med, mean, dev;
};

void runStatistics(std::vector<double> &timings_vector, Statistics &stats)
{
    std::sort(timings_vector.begin(), timings_vector.end());
    stats.min = timings_vector.front();
    stats.max = timings_vector.back();
    stats.med = timings_vector[timings_vector.size() / 2];
    double primary_sum = std::accumulate(timings_vector.begin(), timings_vector.end(), 0.0);
    stats.mean = primary_sum / timings_vector.size();

    double primary_sq_sum = std::inner_product(
        timings_vector.begin(), timings_vector.end(), timings_vector.begin(), 0.0);
    stats.dev = std::sqrt(primary_sq_sum / timings_vector.size() - (stats.mean * stats.mean));
}
} // namespace osrm::tools

std::filesystem::path test_path;

int main(int argc, char *argv[])
{

#ifdef __FreeBSD__
    osrm::util::Log() << "Not supported on FreeBSD";
    return 0;
#endif
#ifdef _WIN32
    osrm::util::Log() << "Not supported on Windows";
    return 0;
#else

    osrm::util::LogPolicy::GetInstance().Unmute();
    if (1 == argc)
    {
        osrm::util::Log(logWARNING) << "usage: " << argv[0] << " /path/on/device";
        return -1;
    }

    test_path = std::filesystem::path(argv[1]);
    test_path /= "osrm.tst";
    osrm::util::Log(logDEBUG) << "temporary file: " << test_path.string();

    // create files for testing
    if (2 == argc)
    {
        // create file to test
        if (std::filesystem::exists(test_path))
        {
            throw osrm::util::exception("Data file already exists: " + test_path.string() +
                                        SOURCE_REF);
        }

        int *random_array = new int[osrm::tools::NUMBER_OF_ELEMENTS];
        std::generate(random_array, random_array + osrm::tools::NUMBER_OF_ELEMENTS, std::rand);
#ifdef __APPLE__
        FILE *fd = fopen(test_path.string().c_str(), "w");
        fcntl(fileno(fd), F_NOCACHE, 1);
        fcntl(fileno(fd), F_RDAHEAD, 0);
        TIMER_START(write_1gb);
        write(fileno(fd), (char *)random_array, osrm::tools::NUMBER_OF_ELEMENTS * sizeof(unsigned));
        TIMER_STOP(write_1gb);
        fclose(fd);
#endif
#ifdef __linux__
        int file_desc =
            open(test_path.string().c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, S_IRWXU);
        if (-1 == file_desc)
        {
            throw osrm::util::exception("Could not open random data file" + test_path.string() +
                                        SOURCE_REF);
        }
        TIMER_START(write_1gb);
        int ret =
            write(file_desc, random_array, osrm::tools::NUMBER_OF_ELEMENTS * sizeof(unsigned));
        if (0 > ret)
        {
            throw osrm::util::exception("could not write random data file" + test_path.string() +
                                        SOURCE_REF);
        }
        TIMER_STOP(write_1gb);
        close(file_desc);
#endif
        delete[] random_array;
        osrm::util::Log(logDEBUG) << "writing raw 1GB took " << TIMER_SEC(write_1gb) << "s";
        osrm::util::Log() << "raw write performance: " << std::setprecision(5) << std::fixed
                          << 1024 * 1024 / TIMER_SEC(write_1gb) << "MB/sec";

        osrm::util::Log(logDEBUG) << "finished creation of random data. Flush disk cache now!";
    }
    else
    {
        // Run Non-Cached I/O benchmarks
        if (!std::filesystem::exists(test_path))
        {
            throw osrm::util::exception("data file does not exist" + SOURCE_REF);
        }

        // volatiles do not get optimized
        osrm::tools::Statistics stats;

#ifdef __APPLE__
        volatile unsigned single_block[1024];
        char *raw_array = new char[osrm::tools::NUMBER_OF_ELEMENTS * sizeof(unsigned)];
        FILE *fd = fopen(test_path.string().c_str(), "r");
        fcntl(fileno(fd), F_NOCACHE, 1);
        fcntl(fileno(fd), F_RDAHEAD, 0);
#endif
#ifdef __linux__
        char *single_block = (char *)memalign(512, 1024 * sizeof(unsigned));

        int file_desc = open(test_path.string().c_str(), O_RDONLY | O_DIRECT | O_SYNC);
        if (-1 == file_desc)
        {
            osrm::util::Log(logDEBUG) << "opened, error: " << strerror(errno);
            return -1;
        }
        char *raw_array = (char *)memalign(512, osrm::tools::NUMBER_OF_ELEMENTS * sizeof(unsigned));
#endif
        TIMER_START(read_1gb);
#ifdef __APPLE__
        read(fileno(fd), raw_array, osrm::tools::NUMBER_OF_ELEMENTS * sizeof(unsigned));
        close(fileno(fd));
        fd = fopen(test_path.string().c_str(), "r");
#endif
#ifdef __linux__
        int ret = read(file_desc, raw_array, osrm::tools::NUMBER_OF_ELEMENTS * sizeof(unsigned));
        osrm::util::Log(logDEBUG) << "read " << ret << " bytes, error: " << strerror(errno);
        close(file_desc);
        file_desc = open(test_path.string().c_str(), O_RDONLY | O_DIRECT | O_SYNC);
        osrm::util::Log(logDEBUG) << "opened, error: " << strerror(errno);
#endif
        TIMER_STOP(read_1gb);

        osrm::util::Log(logDEBUG) << "reading raw 1GB took " << TIMER_SEC(read_1gb) << "s";
        osrm::util::Log() << "raw read performance: " << std::setprecision(5) << std::fixed
                          << 1024 * 1024 / TIMER_SEC(read_1gb) << "MB/sec";

        std::vector<double> timing_results_raw_random;
        osrm::util::Log(logDEBUG) << "running 1000 random I/Os of 4KB";

#ifdef __APPLE__
        fseek(fd, 0, SEEK_SET);
#endif
#ifdef __linux__
        lseek(file_desc, 0, SEEK_SET);
#endif
        // make 1000 random access, time each I/O seperately
        unsigned number_of_blocks = (osrm::tools::NUMBER_OF_ELEMENTS * sizeof(unsigned) - 1) / 4096;
        std::random_device rd;
        std::default_random_engine e1(rd());
        std::uniform_int_distribution<unsigned> uniform_dist(0, number_of_blocks - 1);
        for (unsigned i = 0; i < 1000; ++i)
        {
            unsigned block_to_read = uniform_dist(e1);
            off_t current_offset = block_to_read * 4096;
            TIMER_START(random_access);
#ifdef __APPLE__
            int ret1 = fseek(fd, current_offset, SEEK_SET);
            int ret2 = read(fileno(fd), (char *)&single_block[0], 4096);
#endif

#ifdef __FreeBSD__
            int ret1 = 0;
            int ret2 = 0;
#endif

#ifdef __linux__
            int ret1 = lseek(file_desc, current_offset, SEEK_SET);
            int ret2 = read(file_desc, (char *)single_block, 4096);
#endif
            TIMER_STOP(random_access);
            if (((off_t)-1) == ret1)
            {
                osrm::util::Log(logWARNING) << "offset: " << current_offset;
                osrm::util::Log(logWARNING) << "seek error " << strerror(errno);
                throw osrm::util::exception("seek error" + SOURCE_REF);
            }
            if (-1 == ret2)
            {
                osrm::util::Log(logWARNING) << "offset: " << current_offset;
                osrm::util::Log(logWARNING) << "read error " << strerror(errno);
                throw osrm::util::exception("read error" + SOURCE_REF);
            }
            timing_results_raw_random.push_back(TIMER_SEC(random_access));
        }

        // Do statistics
        osrm::util::Log(logDEBUG) << "running raw random I/O statistics";
        std::ofstream random_csv("random.csv", std::ios::trunc);
        for (unsigned i = 0; i < timing_results_raw_random.size(); ++i)
        {
            random_csv << i << ", " << timing_results_raw_random[i] << std::endl;
        }
        osrm::tools::runStatistics(timing_results_raw_random, stats);

        osrm::util::Log() << "raw random I/O: " << std::setprecision(5) << std::fixed
                          << "min: " << stats.min << "ms, "
                          << "mean: " << stats.mean << "ms, "
                          << "med: " << stats.med << "ms, "
                          << "max: " << stats.max << "ms, "
                          << "dev: " << stats.dev << "ms";

        std::vector<double> timing_results_raw_seq;
#ifdef __APPLE__
        fseek(fd, 0, SEEK_SET);
#endif
#ifdef __linux__
        lseek(file_desc, 0, SEEK_SET);
#endif

        // read every 100th block
        for (unsigned i = 0; i < 1000; ++i)
        {
            off_t current_offset = i * 4096;
            TIMER_START(read_every_100);
#ifdef __APPLE__
            int ret1 = fseek(fd, current_offset, SEEK_SET);
            int ret2 = read(fileno(fd), (char *)&single_block, 4096);
#endif

#ifdef __FreeBSD__
            int ret1 = 0;
            int ret2 = 0;
#endif

#ifdef __linux__
            int ret1 = lseek(file_desc, current_offset, SEEK_SET);

            int ret2 = read(file_desc, (char *)single_block, 4096);
#endif
            TIMER_STOP(read_every_100);
            if (((off_t)-1) == ret1)
            {
                osrm::util::Log(logWARNING) << "offset: " << current_offset;
                osrm::util::Log(logWARNING) << "seek error " << strerror(errno);
                throw osrm::util::exception("seek error" + SOURCE_REF);
            }
            if (-1 == ret2)
            {
                osrm::util::Log(logWARNING) << "offset: " << current_offset;
                osrm::util::Log(logWARNING) << "read error " << strerror(errno);
                throw osrm::util::exception("read error" + SOURCE_REF);
            }
            timing_results_raw_seq.push_back(TIMER_SEC(read_every_100));
        }
#ifdef __APPLE__
        fclose(fd);
        // free(single_element);
        free(raw_array);
// free(single_block);
#endif
#ifdef __linux__
        close(file_desc);
#endif
        // Do statistics
        osrm::util::Log(logDEBUG) << "running sequential I/O statistics";
        // print simple statistics: min, max, median, variance
        std::ofstream seq_csv("sequential.csv", std::ios::trunc);
        for (unsigned i = 0; i < timing_results_raw_seq.size(); ++i)
        {
            seq_csv << i << ", " << timing_results_raw_seq[i] << std::endl;
        }
        osrm::tools::runStatistics(timing_results_raw_seq, stats);
        osrm::util::Log() << "raw sequential I/O: " << std::setprecision(5) << std::fixed
                          << "min: " << stats.min << "ms, "
                          << "mean: " << stats.mean << "ms, "
                          << "med: " << stats.med << "ms, "
                          << "max: " << stats.max << "ms, "
                          << "dev: " << stats.dev << "ms";

        if (std::filesystem::exists(test_path))
        {
            std::filesystem::remove(test_path);
            osrm::util::Log(logDEBUG) << "removing temporary files";
        }
    }
    return EXIT_SUCCESS;
#endif
}
