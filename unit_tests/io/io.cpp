#include "util/io.hpp"
#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <string>

const static std::string IO_TMP_FILE = "test_io.tmp";

BOOST_AUTO_TEST_SUITE(osrm_io)

BOOST_AUTO_TEST_CASE(io_flags)
{
    std::vector<bool> flags_in, flags_out;
    flags_in.resize(53);
    for (std::size_t i = 0; i < flags_in.size(); ++i)
        flags_in[i] = ((i % 2) == 1);

    osrm::util::serializeFlags(IO_TMP_FILE, flags_in);
    osrm::util::deserializeFlags(IO_TMP_FILE, flags_out);

    BOOST_REQUIRE_EQUAL(flags_in.size(), flags_out.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(flags_out.begin(), flags_out.end(), flags_in.begin(),
                                  flags_in.end());
}

BOOST_AUTO_TEST_CASE(io_data)
{
    std::vector<int> data_in, data_out;
    data_in.resize(53);
    for (std::size_t i = 0; i < data_in.size(); ++i)
        data_in[i] = i;

    osrm::util::serializeVector(IO_TMP_FILE, data_in);
    osrm::util::deserializeVector(IO_TMP_FILE, data_out);

    BOOST_REQUIRE_EQUAL(data_in.size(), data_out.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(data_out.begin(), data_out.end(), data_in.begin(),
                                  data_in.end());
}

BOOST_AUTO_TEST_SUITE_END()
