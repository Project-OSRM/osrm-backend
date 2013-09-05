/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#include "../Util/OSRMException.h"
#include "../Util/SimpleLogger.h"
#include "../Util/TimingUtil.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/ref.hpp>

#include <cmath>

#include <algorithm>
#include <numeric>
#include <vector>

const unsigned number_of_elements = 268435456;

struct Statistics { double min, max, med, mean, dev; };

void RunStatistics(std::vector<double> & timings_vector, Statistics & stats) {
    std::sort(timings_vector.begin(), timings_vector.end());
    stats.min = timings_vector.front();
    stats.max = timings_vector.back();
    stats.med = timings_vector[timings_vector.size()/2];
    double primary_sum =    std::accumulate(
                                timings_vector.begin(),
                                timings_vector.end(),
                                0.0
                            );
    stats.mean = primary_sum / timings_vector.size();

    double primary_sq_sum = std::inner_product( timings_vector.begin(),
                                timings_vector.end(),
                                timings_vector.begin(),
                                0.0
                            );
     stats.dev = std::sqrt(
        primary_sq_sum / timings_vector.size() - (stats.mean * stats.mean)
    );
}

int main (int argc, char * argv[]) {
    LogPolicy::GetInstance().Unmute();

    SimpleLogger().Write(logDEBUG) << "starting up engines, compiled at " <<
        __DATE__ << ", " __TIME__;

    if( 1 == argc ) {
        SimpleLogger().Write(logWARNING) <<
            "usage: " << argv[0] << " /path/on/device";
        return -1;
    }

    boost::filesystem::path test_path = boost::filesystem::path(argv[1]);
    test_path /= "osrm.tst";
    SimpleLogger().Write(logDEBUG) << "temporary file: " << test_path.string();

    try {
        //create file to test
        if( boost::filesystem::exists(test_path) ) {
            boost::filesystem::remove(test_path);
            SimpleLogger().Write() << "removing previous files";
        }

        SimpleLogger().Write(logDEBUG) << "allocating 2GB in RAM";
        std::vector<unsigned> primary_vector(number_of_elements, 0);

        SimpleLogger().Write(logDEBUG) << "fill primary vector with data";
        std::srand ( 37337 );
        std::generate (primary_vector.begin(), primary_vector.end(), std::rand);

        SimpleLogger().Write(logDEBUG) <<
            "writing " << number_of_elements*sizeof(unsigned) << " bytes";
        //write 1GB to random filename, time everything
        boost::filesystem::ofstream test_stream(test_path, std::ios::binary);
        double time1 = get_timestamp();
        test_stream.write(
            (char *)&primary_vector[0],
            number_of_elements*sizeof(unsigned)
        );
        test_stream.flush();
        test_stream.close();
        double time2 = get_timestamp();
        SimpleLogger().Write(logDEBUG) <<
            "writing 1GB took " << (time2-time1)*1000 << "ms";
        SimpleLogger().Write() << "Write performance: " <<
            1024*1024/((time2-time1)*1000) << "MB/sec";
        SimpleLogger().Write(logDEBUG) <<
            "reading " << number_of_elements*sizeof(unsigned) << " bytes";

        //read and check 1GB of random data, time everything
        std::vector<unsigned> secondary_vector(number_of_elements, 0);
        boost::filesystem::ifstream read_stream( test_path, std::ios::binary );
        time1 = get_timestamp();
        read_stream.read(
            (char *)&secondary_vector[0],
            number_of_elements*sizeof(unsigned)
        );
        read_stream.sync();
        read_stream.close();
        time2 = get_timestamp();

        SimpleLogger().Write(logDEBUG) <<
            "reading 1GB took " << (time2-time1)*1000 << "ms";
        SimpleLogger().Write() << "Read performance: " <<
            1024*1024/((time2-time1)*1000) << "MB/sec";
        SimpleLogger().Write(logDEBUG) << "checking data for correctness";

        if(!std::equal(
                primary_vector.begin(),
                primary_vector.end(),
                secondary_vector.begin()
            )
        ) {
            throw OSRMException("data file is corrupted");
        }

        //removing any temporary data
        std::vector<unsigned>().swap(primary_vector);
        std::vector<unsigned>().swap(secondary_vector);

        //reopening read stream
        read_stream.open(test_path, std::ios::binary);

        SimpleLogger().Write(logDEBUG) << "running 1000+/-1 gapped I/Os of 4B";
        std::vector<double> timing_results_gapped;
        volatile unsigned single_element = 0;
        //read every 268435'th byte, time each I/O seperately
        for(
            int i = number_of_elements;
            i > 0 ;
            i-=(number_of_elements/1000)
        ) {
            time1 = get_timestamp();
            read_stream.seekg(i*sizeof(unsigned));
            read_stream.read( (char*)&single_element, sizeof(unsigned));
            time2 = get_timestamp();
            timing_results_gapped.push_back((time2-time1));
        }

        //Do statistics
        SimpleLogger().Write(logDEBUG) << "running gapped I/O statistics";
        //print simple statistics: min, max, median, variance
        Statistics primary_stats;
        RunStatistics(timing_results_gapped, primary_stats);
        SimpleLogger().Write() << "gapped I/O: " <<
            "min: "  << primary_stats.min*1000  << "ms, " <<
            "mean: " << primary_stats.mean*1000 << "ms, " <<
            "med: "  << primary_stats.med*1000  << "ms, " <<
            "max: "  << primary_stats.max*1000  << "ms, " <<
            "dev: "  << primary_stats.dev*1000  << "ms";

        std::vector<double> timing_results_random;
        volatile unsigned temp_array[1024]; //compilers dont optimize volatiles
        SimpleLogger().Write(logDEBUG) << "running 1000 random I/Os of 4KB";

        //make 1000 random access, time each I/O seperately
        for(unsigned i = 0; i < 1000; ++i) {
            unsigned element_to_read = std::rand()%(number_of_elements-1024);
            time1 = get_timestamp();
            read_stream.seekg(element_to_read*sizeof(unsigned));
            read_stream.read( (char*)&temp_array[0], 1024*sizeof(unsigned));
            time2 = get_timestamp();
            timing_results_random.push_back((time2-time1));
        }
        read_stream.close();

        // Do statistics
        SimpleLogger().Write(logDEBUG) << "running random I/O statistics";
        Statistics secondary_stats;
        RunStatistics(timing_results_random, secondary_stats);

        SimpleLogger().Write() << "random I/O: "  <<
            "min: "  << secondary_stats.min*1000  << "ms, " <<
            "mean: " << secondary_stats.mean*1000 << "ms, " <<
            "med: "  << secondary_stats.med*1000  << "ms, " <<
            "max: "  << secondary_stats.max*1000  << "ms, " <<
            "dev: "  << secondary_stats.dev*1000  << "ms";

        if( boost::filesystem::exists(test_path) ) {
            boost::filesystem::remove(test_path);
            SimpleLogger().Write(logDEBUG) << "removing temporary files";
        }
    } catch ( const std::exception & e ) {
        SimpleLogger().Write(logWARNING) << "caught exception: " << e.what();
        SimpleLogger().Write(logWARNING) << "cleaning up, and exiting";
        if(boost::filesystem::exists(test_path)) {
            boost::filesystem::remove(test_path);
            SimpleLogger().Write(logWARNING) << "removing temporary files";
        }
        return -1;
    }
    return 0;
}
