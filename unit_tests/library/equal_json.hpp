#ifndef UNIT_TESTS_JSON_EQUAL
#define UNIT_TESTS_JSON_EQUAL

#include <boost/test/unit_test.hpp>

#include "osrm/json_container.hpp"
#include "util/json_deep_compare.hpp"

inline boost::test_tools::predicate_result compareJSON(const osrm::util::json::Value &reference,
                                                       const osrm::util::json::Value &result)
{
    std::string reason;
    auto is_same = osrm::util::json::compare(reference, result, reason);
    if (!is_same)
    {
        boost::test_tools::predicate_result res(false);

        res.message() << reason;

        return res;
    }

    return true;
}

#define CHECK_EQUAL_JSON(reference, result) BOOST_CHECK(compareJSON(reference, result));

#endif
