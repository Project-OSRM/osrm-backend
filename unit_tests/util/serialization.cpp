#include "storage/serialization.hpp"

#include <boost/filesystem.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(serialization_test)

using namespace osrm;
using namespace osrm::util;
using namespace osrm::storage::io;
using namespace osrm::storage::serialization;

BOOST_AUTO_TEST_CASE(pack_test)
{
    std::vector<bool> v = {0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1};

    BOOST_CHECK_EQUAL(osrm::storage::serialization::packBits(v, 0, 8), 0x2e);
    BOOST_CHECK_EQUAL(osrm::storage::serialization::packBits(v, 5, 7), 0x65);
    BOOST_CHECK_EQUAL(osrm::storage::serialization::packBits(v, 6, 8), 0x95);
    BOOST_CHECK_EQUAL(osrm::storage::serialization::packBits(v, 11, 1), 0x01);
}

BOOST_AUTO_TEST_CASE(unpack_test)
{
    std::vector<bool> v(14), expected = {0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1};

    osrm::storage::serialization::unpackBits(v, 0, 8, 0x2e);
    osrm::storage::serialization::unpackBits(v, 5, 7, 0x65);
    osrm::storage::serialization::unpackBits(v, 6, 8, 0x95);
    osrm::storage::serialization::unpackBits(v, 11, 1, 0x01);
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), expected.begin(), expected.end());
}

struct SerializationFixture
{
    SerializationFixture() : temporary_file(boost::filesystem::unique_path()) {}
    ~SerializationFixture() { remove(temporary_file); }

    boost::filesystem::path temporary_file;
};

BOOST_AUTO_TEST_CASE(serialize_bool_vector)
{
    SerializationFixture fixture;
    {
        std::vector<std::vector<bool>> data = {
            {}, {0}, {1, 1, 1}, {1, 1, 0, 0, 1, 1, 0, 0}, {1, 1, 0, 0, 1, 1, 0, 0, 1}};
        for (const auto &v : data)
        {
            {
                FileWriter writer(fixture.temporary_file, FileWriter::GenerateFingerprint);
                write(writer, v);
            }
            std::vector<bool> result;
            FileReader reader(fixture.temporary_file, FileReader::VerifyFingerprint);
            read(reader, result);
            BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), result.begin(), result.end());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
