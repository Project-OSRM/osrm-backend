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

        std::vector<std::vector<unsigned>> data = {std::vector<unsigned>{0, 10, 24, 100, 2, 100},
                                                   std::vector<unsigned>{1, 12, 15},
                                                   std::vector<unsigned>{10, 10, 10}};

        for (const auto &v : data)
        {
            TestRangeTable reference{v};
            {
                storage::tar::FileWriter writer(tmp.path,
                                                storage::tar::FileWriter::GenerateFingerprint);
                util::serialization::write(writer, "my_range_table", reference);
            }

            TestRangeTable result;
            storage::tar::FileReader reader(tmp.path, storage::tar::FileReader::VerifyFingerprint);
            util::serialization::read(reader, "my_range_table", result);

            for (auto index : util::irange<std::size_t>(0, v.size()))
            {
                CHECK_EQUAL_COLLECTIONS(result.GetRange(index), reference.GetRange(index));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(tar_serialize_packed_vector)
{
    TemporaryFile tmp;
    {
        using TestPackedVector = PackedVector<std::uint64_t, 33>;

        std::vector<TestPackedVector> data = {{1597322404,
                                               1939964443,
                                               2112255763,
                                               1432114613,
                                               1067854538,
                                               352118606,
                                               1782436840,
                                               1909002904,
                                               165344818},
                                              {0, 1, 2, 3}};

        for (const auto &v : data)
        {
            {
                storage::tar::FileWriter writer(tmp.path,
                                                storage::tar::FileWriter::GenerateFingerprint);
                util::serialization::write(writer, "my_packed_vector", v);
            }

            TestPackedVector result;
            storage::tar::FileReader reader(tmp.path, storage::tar::FileReader::VerifyFingerprint);
            util::serialization::read(reader, "my_packed_vector", result);

            CHECK_EQUAL_COLLECTIONS(result, v);
        }
    }
}

BOOST_AUTO_TEST_CASE(tar_serialize_variable_indexed_data)
{
    TemporaryFile tmp;
    {
        using TestIndexedData = IndexedData<VariableGroupBlock<16, std::string>>;

        std::vector<std::vector<unsigned>> offset_data = {
            {5, 8, 8},
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
        std::vector<std::string> char_data = {
            "HalloFoo", "ABCDEFGHIJKLMNOPQR", "ABCDEFGHIJKLMNOP",
        };

        for (const auto i : util::irange<std::size_t>(0, offset_data.size()))
        {
            TestIndexedData indexed{
                offset_data[i].begin(), offset_data[i].end(), char_data[i].begin()};
            {
                storage::tar::FileWriter writer(tmp.path,
                                                storage::tar::FileWriter::GenerateFingerprint);
                util::serialization::write(writer, "my_indexed_data", indexed);
            }

            TestIndexedData result;
            storage::tar::FileReader reader(tmp.path, storage::tar::FileReader::VerifyFingerprint);
            util::serialization::read(reader, "my_indexed_data", result);

            for (auto j : util::irange<std::size_t>(0, offset_data[i].size() - 1))
            {
                BOOST_CHECK_EQUAL(indexed.at(j), result.at(j));
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
