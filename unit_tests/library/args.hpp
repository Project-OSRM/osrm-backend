#ifndef OSRM_UNIT_TEST_ARGS
#define OSRM_UNIT_TEST_ARGS

#include "util/log.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include <iostream>
#include <string>
#include <vector>

inline std::vector<std::string> get_args()
{
    osrm::util::LogPolicy::GetInstance().Unmute();
    if ((boost::unit_test::framework::master_test_suite().argc != 2) ||
        (!boost::filesystem::is_regular_file(
            boost::unit_test::framework::master_test_suite().argv[1])))
    {
        osrm::util::Log(logERROR) << "Please provide valid input osrm file";
        osrm::util::Log(logERROR) << "Usage: "
                                  << boost::unit_test::framework::master_test_suite().argv[0]
                                  << " /path/to/input_osrm_file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Split off argv[0], store actual positional arguments in args
    const auto argc = boost::unit_test::framework::master_test_suite().argc - 1;
    const auto argv = boost::unit_test::framework::master_test_suite().argv + 1;

    return {argv, argv + argc};
}

#endif
