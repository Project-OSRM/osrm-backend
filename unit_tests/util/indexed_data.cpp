#include "util/indexed_data.hpp"
#include "util/exception.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <vector>

BOOST_AUTO_TEST_SUITE(indexed_data)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(check_variable_group_block_bitops)
{
    VariableGroupBlock<16> variable_group_block;
    BOOST_CHECK_EQUAL(variable_group_block.sum2bits(0xe4), 6);
    BOOST_CHECK_EQUAL(variable_group_block.sum2bits(0x11111111), 8);
    BOOST_CHECK_EQUAL(variable_group_block.sum2bits(0x55555555), 16);
    BOOST_CHECK_EQUAL(variable_group_block.sum2bits(0xffffffff), 48);

    BOOST_CHECK_EQUAL(variable_group_block.log256(0), 0);
    BOOST_CHECK_EQUAL(variable_group_block.log256(1), 1);
    BOOST_CHECK_EQUAL(variable_group_block.log256(255), 1);
    BOOST_CHECK_EQUAL(variable_group_block.log256(256), 2);
    BOOST_CHECK_EQUAL(variable_group_block.log256(1024), 2);
    BOOST_CHECK_EQUAL(variable_group_block.log256(16777215), 3);
}

template <typename IndexedData, typename Offsets, typename Data>
void test_rw(const Offsets &offsets, const Data &data)
{
    std::stringstream sstr;
    IndexedData indexed_data;
    indexed_data.write(sstr, offsets.begin(), offsets.end(), data.begin());

    const std::string str = sstr.str();

#if 0
    std::cout << "\n" << typeid(IndexedData).name() << "\nsaved size = " << str.size() << "\n";
    for (auto c : str)
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)((unsigned char)c) << " ";
    std::cout << std::dec << "\n";
#endif

    indexed_data.reset(str.c_str(), str.c_str() + str.size());

    for (std::size_t index = 0; index < offsets.size() - 1; ++index)
    {
        typename IndexedData::ResultType expected_result(&data[offsets[index]],
                                                         &data[offsets[index + 1]]);
        BOOST_CHECK_EQUAL(expected_result, indexed_data.at(index));
    }
}

BOOST_AUTO_TEST_CASE(check_group_blocks_with_different_sizes)
{

    std::vector<std::string> str = {
        "",     "A", "bb", "ccc", "dDDd", "E", "ff", "ggg", "hhhh", "I", "jj", "",  "kkk",
        "llll", "M", "nn", "ooo", "pppp", "q", "r",  "S",   "T",    "",  "u",  "V", "W",
        "X",    "Y", "Z",  "",    "",     "",  "",   "",    "",     "",  "0",  ""};

    std::vector<unsigned char> name_char_data;
    std::vector<std::uint32_t> name_offsets;
    for (auto s : str)
    {
        name_offsets.push_back(name_char_data.size());
        std::copy(s.begin(), s.end(), std::back_inserter(name_char_data));
    }
    name_offsets.push_back(name_char_data.size());

    test_rw<IndexedData<VariableGroupBlock<0, std::string>>>(name_offsets, name_char_data);
    test_rw<IndexedData<VariableGroupBlock<1, std::string>>>(name_offsets, name_char_data);
    test_rw<IndexedData<VariableGroupBlock<16, std::string>>>(name_offsets, name_char_data);

    test_rw<IndexedData<FixedGroupBlock<0, std::string>>>(name_offsets, name_char_data);
    test_rw<IndexedData<FixedGroupBlock<1, std::string>>>(name_offsets, name_char_data);
    test_rw<IndexedData<FixedGroupBlock<16, std::string>>>(name_offsets, name_char_data);
    test_rw<IndexedData<FixedGroupBlock<32, std::string>>>(name_offsets, name_char_data);
    test_rw<IndexedData<FixedGroupBlock<128, std::string>>>(name_offsets, name_char_data);
}

BOOST_AUTO_TEST_CASE(check_1001_pandas)
{
    std::vector<unsigned char> name_char_data;
    std::vector<std::uint32_t> name_offsets;

    const std::string panda = "🐼";
    name_offsets.push_back(0);
    for (std::size_t i = 0; i < 1000; ++i)
        std::copy(panda.begin(), panda.end(), std::back_inserter(name_char_data));
    name_offsets.push_back(name_char_data.size());
    std::copy(panda.begin(), panda.end(), std::back_inserter(name_char_data));
    name_offsets.push_back(name_char_data.size());

    test_rw<IndexedData<VariableGroupBlock<16, std::string>>>(name_offsets, name_char_data);
}

BOOST_AUTO_TEST_CASE(check_different_sizes)
{
    for (std::size_t num_strings = 0; num_strings < 256; ++num_strings)
    {
        std::vector<unsigned char> name_char_data;
        std::vector<std::uint32_t> name_offsets;

        const std::string canoe = "🛶";
        name_offsets.push_back(0);
        for (std::size_t i = 0; i < num_strings; ++i)
        {
            std::copy(canoe.begin(), canoe.end(), std::back_inserter(name_char_data));
            name_offsets.push_back(name_char_data.size());
        }

        test_rw<IndexedData<VariableGroupBlock<16, std::string>>>(name_offsets, name_char_data);
        test_rw<IndexedData<FixedGroupBlock<16, std::string>>>(name_offsets, name_char_data);
    }
}

BOOST_AUTO_TEST_CASE(check_max_size)
{
    std::vector<unsigned char> name_data(0x1000000, '#');
    std::vector<std::uint32_t> name_offsets;

    auto test_variable = [&name_offsets, &name_data]() {
        test_rw<IndexedData<VariableGroupBlock<16, std::string>>>(name_offsets, name_data);
    };
    auto test_fixed = [&name_offsets, &name_data]() {
        test_rw<IndexedData<FixedGroupBlock<16, std::string>>>(name_offsets, name_data);
    };

    name_offsets = {0, 0x1000000};
    BOOST_CHECK_THROW(test_variable(), osrm::util::exception);
    name_offsets = {0, 0x1000000 - 1};
    test_variable();

    name_offsets = {0, 256};
    BOOST_CHECK_THROW(test_fixed(), osrm::util::exception);
    name_offsets = {0, 255};
    test_fixed();
}

BOOST_AUTO_TEST_CASE(check_corrupted_memory)
{
    std::vector<unsigned char> buf;

    auto test_variable = [&buf]() {
        IndexedData<VariableGroupBlock<16, std::vector<unsigned char>>> indexed_data;
        indexed_data.reset(&buf[0], &buf[buf.size()]);
        const auto result = indexed_data.at(0);
        return std::string(reinterpret_cast<const char *>(&result[0]), result.size());
    };

    // Use LE internal representation
    buf = {0, 42};
    BOOST_CHECK_THROW(test_variable(), osrm::util::exception);

    buf = {1, 0, 0, 0, 0};
    BOOST_CHECK_THROW(test_variable(), osrm::util::exception);

    buf = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 42};
    BOOST_CHECK_THROW(test_variable(), osrm::util::exception);

    buf = {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 42};
    BOOST_CHECK_THROW(test_variable(), osrm::util::exception);

    buf = {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 4, 0xF0, 0x9F, 0x90, 0xBC};
    BOOST_CHECK_EQUAL(test_variable(), "🐼");
}

BOOST_AUTO_TEST_CASE(check_string_view)
{
    std::stringstream sstr;
    std::string name_data = "hellostringview";
    std::vector<std::uint32_t> name_offsets = {0, 5, 11, 15};

    IndexedData<VariableGroupBlock<16, StringView>> indexed_data;
    indexed_data.write(sstr, name_offsets.begin(), name_offsets.end(), name_data.begin());

    const std::string str = sstr.str();
    indexed_data.reset(str.c_str(), str.c_str() + str.size());

    BOOST_CHECK_EQUAL(indexed_data.at(0), "hello");
    BOOST_CHECK_EQUAL(indexed_data.at(1), "string");
    BOOST_CHECK_EQUAL(indexed_data.at(2), "view");
}

BOOST_AUTO_TEST_SUITE_END()
