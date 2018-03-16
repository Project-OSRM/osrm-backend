#include "util/serialization.hpp"

#include "../common/range_tools.hpp"
#include "../common/temporary_file.hpp"

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(serialization)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(tar_serialize_range_table)
{
    TemporaryFile tmp;
    {
        constexpr unsigned BLOCK_SIZE = 16;
        using TestRangeTable = RangeTable<BLOCK_SIZE, osrm::storage::Ownership::Container>;

        std::vector<TestRangeTable> data = {
        };

        for (const auto &v : data)
        {
            {
                storage::tar::FileWriter writer(tmp.path, storage::tar::FileWriter::GenerateFingerprint);
                util::serialization::write(writer, "my_range_table", v);
            }

            TestRangeTable result;
            storage::tar::FileReader reader(tmp.path, storage::tar::FileReader::VerifyFingerprint);
            util::serialization::read(reader, "my_range_table", result);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
