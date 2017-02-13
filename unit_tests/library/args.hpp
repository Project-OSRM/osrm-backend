#ifndef OSRM_UNIT_TEST_ARGS
#define OSRM_UNIT_TEST_ARGS

#include <string>
#include <vector>
#include <iostream>

inline std::vector<std::string> get_args()
{
    // Split off argv[0], store actual positional arguments in args
    const auto argc = boost::unit_test::framework::master_test_suite().argc - 1;
    const auto argv = boost::unit_test::framework::master_test_suite().argv + 1;

    if (argc == 0)
    {
        std::cout << "You must provide a path to the test data, please see the unit testing docs" << std::endl;
    }

    return {argv, argv + argc};
}

#endif
