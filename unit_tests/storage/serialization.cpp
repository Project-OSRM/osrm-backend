#include "storage/serialization.hpp"

#include "../common/range_tools.hpp"
#include "../common/temporary_file.hpp"

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(serialization)

using namespace osrm;
using namespace osrm::storage;

BOOST_AUTO_TEST_CASE(pack_test)
{
    std::vector<bool> v = {0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1};

    BOOST_CHECK_EQUAL(storage::serialization::detail::packBits(v, 0, 8), 0x2e);
    BOOST_CHECK_EQUAL(storage::serialization::detail::packBits(v, 5, 7), 0x65);
    BOOST_CHECK_EQUAL(storage::serialization::detail::packBits(v, 6, 8), 0x95);
    BOOST_CHECK_EQUAL(storage::serialization::detail::packBits(v, 11, 1), 0x01);
}

BOOST_AUTO_TEST_CASE(unpack_test)
{
    std::vector<bool> v(14), expected = {0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1};

    storage::serialization::detail::unpackBits(v, 0, 8, 0x2e);
    storage::serialization::detail::unpackBits(v, 5, 7, 0x65);
    storage::serialization::detail::unpackBits(v, 6, 8, 0x95);
    storage::serialization::detail::unpackBits(v, 11, 1, 0x01);
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(bin_serialize_bool_vector)
{
    TemporaryFile tmp;
    {
        std::vector<std::vector<bool>> data = {
            {}, {0}, {1, 1, 1}, {1, 1, 0, 0, 1, 1, 0, 0}, {1, 1, 0, 0, 1, 1, 0, 0, 1}};
        for (const auto &v : data)
        {
            {
                io::FileWriter writer(tmp.path, io::FileWriter::GenerateFingerprint);
                storage::serialization::write(writer, v);
            }
            std::vector<bool> result;
            io::FileReader reader(tmp.path, io::FileReader::VerifyFingerprint);
            storage::serialization::read(reader, result);
            BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), result.begin(), result.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(tar_serialize_bool_vector)
{
    TemporaryFile tmp;
    {
        std::vector<std::vector<bool>> data = {
            {}, {0}, {1, 1, 1}, {1, 1, 0, 0, 1, 1, 0, 0}, {1, 1, 0, 0, 1, 1, 0, 0, 1}};
        for (const auto &v : data)
        {
            {
                tar::FileWriter writer(tmp.path, tar::FileWriter::GenerateFingerprint);
                storage::serialization::write(writer, "my_boolean_vector", v);
            }
            std::vector<bool> result;
            tar::FileReader reader(tmp.path, tar::FileReader::VerifyFingerprint);
            storage::serialization::read(reader, "my_boolean_vector", result);
            BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), result.begin(), result.end());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
