#include <boost/test/unit_test.hpp>

#include "osrm/partitioner.hpp"
#include "osrm/partitioner_config.hpp"

#include <thread>

BOOST_AUTO_TEST_SUITE(library_partition)

BOOST_AUTO_TEST_CASE(test_partition_with_invalid_config)
{
    using namespace osrm;

    osrm::PartitionerConfig config;
    config.requested_num_threads = std::thread::hardware_concurrency();
    BOOST_CHECK_THROW(osrm::partition(config),
                      std::exception); // including osrm::util::exception, etc.
}

BOOST_AUTO_TEST_SUITE_END()
