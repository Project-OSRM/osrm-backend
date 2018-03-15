#include "storage/tar.hpp"

#include "../common/range_tools.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(tar)

using namespace osrm;

BOOST_AUTO_TEST_CASE(list_tar_file)
{
    storage::tar::FileReader reader(TEST_DATA_DIR "/tar_test.tar");

    std::vector<storage::tar::FileReader::TarEntry> file_list;
    reader.List(std::back_inserter(file_list));

    auto reference_0 = storage::tar::FileReader::TarEntry{"foo_1.txt", 4};
    auto reference_1 = storage::tar::FileReader::TarEntry{"bla/foo_2.txt", 4};
    auto reference_2 = storage::tar::FileReader::TarEntry{"foo_3.txt", 4};

    BOOST_CHECK_EQUAL(std::get<0>(file_list[0]), std::get<0>(reference_0));
    BOOST_CHECK_EQUAL(std::get<1>(file_list[0]), std::get<1>(reference_0));
    BOOST_CHECK_EQUAL(std::get<0>(file_list[1]), std::get<0>(reference_1));
    BOOST_CHECK_EQUAL(std::get<1>(file_list[1]), std::get<1>(reference_1));
    BOOST_CHECK_EQUAL(std::get<0>(file_list[2]), std::get<0>(reference_2));
    BOOST_CHECK_EQUAL(std::get<1>(file_list[2]), std::get<1>(reference_2));
}

BOOST_AUTO_TEST_CASE(read_tar_file)
{
    storage::tar::FileReader reader(TEST_DATA_DIR "/tar_test.tar");

    char result_0[4];
    reader.ReadInto("foo_1.txt", result_0, 4);

    // Note: This is an out-of-order read, foo_3 comes after bla/foo_2 in the tar file
    char result_1[4];
    reader.ReadInto("foo_3.txt", result_1, 4);

    char result_2[4];
    reader.ReadInto("bla/foo_2.txt", result_2, 4);

    BOOST_CHECK_EQUAL(std::string(result_0, 4), std::string("bla\n"));
    BOOST_CHECK_EQUAL(std::string(result_1, 4), std::string("foo\n"));
    BOOST_CHECK_EQUAL(std::string(result_2, 4), std::string("baz\n"));
}

BOOST_AUTO_TEST_CASE(write_tar_file)
{
    boost::filesystem::path tmp_path(TEST_DATA_DIR "/tar_write_test.tar");

    std::uint64_t single_64bit_integer = 0xDEADBEEFAABBCCDD;
    std::uint32_t single_32bit_integer = 0xDEADBEEF;

    std::vector<std::uint32_t> vector_32bit = {0, 1, 2, 3, 4, 1 << 30, 0, 1 << 22, 0xFFFFFFFF};
    std::vector<std::uint64_t> vector_64bit = {
        0, 1, 2, 3, 4, 1ULL << 62, 0, 1 << 22, 0xFFFFFFFFFFFFFFFF};

    {
        storage::tar::FileWriter writer(tmp_path);
        writer.WriteOne("foo/single_64bit_integer", single_64bit_integer);
        writer.WriteOne("bar/single_32bit_integer", single_32bit_integer);
        writer.WriteElementCount64("baz/bla/64bit_vector", vector_64bit.size());
        writer.WriteFrom("baz/bla/64bit_vector", vector_64bit.data(), vector_64bit.size());
        writer.WriteElementCount64("32bit_vector", vector_32bit.size());
        writer.WriteFrom("32bit_vector", vector_32bit.data(), vector_32bit.size());
    }

    storage::tar::FileReader reader(tmp_path);

    BOOST_CHECK_EQUAL(reader.ReadOne<std::uint32_t>("bar/single_32bit_integer"),
                      single_32bit_integer);
    BOOST_CHECK_EQUAL(reader.ReadOne<std::uint64_t>("foo/single_64bit_integer"),
                      single_64bit_integer);

    std::vector<std::uint64_t> result_64bit_vector(
        reader.ReadElementCount64("baz/bla/64bit_vector"));
    reader.ReadInto("baz/bla/64bit_vector", result_64bit_vector.data(), result_64bit_vector.size());
    std::vector<std::uint32_t> result_32bit_vector(reader.ReadElementCount64("32bit_vector"));
    reader.ReadInto("32bit_vector", result_32bit_vector.data(), result_32bit_vector.size());

    CHECK_EQUAL_COLLECTIONS(result_64bit_vector, vector_64bit);
    CHECK_EQUAL_COLLECTIONS(result_32bit_vector, vector_32bit);
}

BOOST_AUTO_TEST_SUITE_END()
