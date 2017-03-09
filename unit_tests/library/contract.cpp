#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "osrm/contractor.hpp"
#include "osrm/contractor_config.hpp"

#include <tbb/task_scheduler_init.h> // default_num_threads

BOOST_AUTO_TEST_SUITE(library_contract)

BOOST_AUTO_TEST_CASE(test_contract_with_invalid_config)
{
    using namespace osrm;

    osrm::ContractorConfig config;
    config.requested_num_threads = tbb::task_scheduler_init::default_num_threads();
    BOOST_CHECK_THROW(osrm::contract(config),
                      std::exception); // including osrm::util::exception, etc.
}

BOOST_AUTO_TEST_SUITE_END()
