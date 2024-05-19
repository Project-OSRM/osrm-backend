#include "extractor/name_table.hpp"
#include "util/exception.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <vector>

BOOST_AUTO_TEST_SUITE(name_table)

using namespace osrm;
using namespace osrm::extractor;

NameTable::IndexedData PrapareNameTableData(std::vector<std::string> &data, bool fill_all)
{
    NameTable::IndexedData indexed_data;
    std::vector<unsigned char> name_char_data;
    std::vector<std::uint32_t> name_offsets;

    for (auto s : data)
    {
        name_offsets.push_back(name_char_data.size());
        std::copy(s.begin(), s.end(), std::back_inserter(name_char_data));

        if (fill_all)
        {
            std::string tmp;

            tmp = s + "_des";
            name_offsets.push_back(name_char_data.size());
            std::copy(tmp.begin(), tmp.end(), std::back_inserter(name_char_data));

            tmp = s + "_pro";
            name_offsets.push_back(name_char_data.size());
            std::copy(tmp.begin(), tmp.end(), std::back_inserter(name_char_data));

            tmp = s + "_ref";
            name_offsets.push_back(name_char_data.size());
            std::copy(tmp.begin(), tmp.end(), std::back_inserter(name_char_data));

            tmp = s + "_ext";
            name_offsets.push_back(name_char_data.size());
            std::copy(tmp.begin(), tmp.end(), std::back_inserter(name_char_data));
        }
        else
        {
            name_offsets.push_back(name_char_data.size());
            name_offsets.push_back(name_char_data.size());
            name_offsets.push_back(name_char_data.size());
            name_offsets.push_back(name_char_data.size());
        }
    }
    name_offsets.push_back(name_char_data.size());

    return NameTable::IndexedData(name_offsets.begin(), name_offsets.end(), name_char_data.begin());
}

BOOST_AUTO_TEST_CASE(check_name_table_fill)
{
    std::vector<std::string> expected_names = {
        "",     "A", "check_name", "ccc", "dDDd", "E", "ff", "ggg", "hhhh", "I", "jj", "",  "kkk",
        "llll", "M", "nn",         "ooo", "pppp", "q", "r",  "S",   "T",    "",  "u",  "V", "W",
        "X",    "Y", "Z",          "",    "",     "",  "",   "",    "",     "",  "0",  ""};

    auto data = PrapareNameTableData(expected_names, true);
    NameTable name_table{data};

    for (std::size_t index = 0; index < expected_names.size(); ++index)
    {
        const NameID id = 5 * index;
        BOOST_CHECK_EQUAL(name_table.GetNameForID(id), expected_names[index]);
        BOOST_CHECK_EQUAL(name_table.GetRefForID(id), expected_names[index] + "_ref");
        BOOST_CHECK_EQUAL(name_table.GetDestinationsForID(id), expected_names[index] + "_des");
        BOOST_CHECK_EQUAL(name_table.GetPronunciationForID(id), expected_names[index] + "_pro");
        BOOST_CHECK_EQUAL(name_table.GetExitsForID(id), expected_names[index] + "_ext");
    }
}

BOOST_AUTO_TEST_CASE(check_name_table_nofill)
{
    std::vector<std::string> expected_names = {
        "",     "A", "check_name", "ccc", "dDDd", "E", "ff", "ggg", "hhhh", "I", "jj", "",  "kkk",
        "llll", "M", "nn",         "ooo", "pppp", "q", "r",  "S",   "T",    "",  "u",  "V", "W",
        "X",    "Y", "Z",          "",    "",     "",  "",   "",    "",     "",  "0",  ""};

    auto data = PrapareNameTableData(expected_names, false);
    NameTable name_table{data};

    // CALLGRIND_START_INSTRUMENTATION;
    for (std::size_t index = 0; index < expected_names.size(); ++index)
    {
        const NameID id = 5 * index;
        BOOST_CHECK_EQUAL(name_table.GetNameForID(id), expected_names[index]);
        BOOST_CHECK(name_table.GetRefForID(id).empty());
        BOOST_CHECK(name_table.GetDestinationsForID(id).empty());
        BOOST_CHECK(name_table.GetPronunciationForID(id).empty());
        BOOST_CHECK(name_table.GetExitsForID(id).empty());
    }
    // CALLGRIND_STOP_INSTRUMENTATION;
}

BOOST_AUTO_TEST_CASE(check_invalid_ids)
{
    NameTable name_table;
    BOOST_CHECK_EQUAL(name_table.GetNameForID(INVALID_NAMEID), "");
    BOOST_CHECK_EQUAL(name_table.GetRefForID(INVALID_NAMEID), "");
    BOOST_CHECK_EQUAL(name_table.GetDestinationsForID(INVALID_NAMEID), "");
    BOOST_CHECK_EQUAL(name_table.GetPronunciationForID(INVALID_NAMEID), "");
    BOOST_CHECK_EQUAL(name_table.GetExitsForID(INVALID_NAMEID), "");
}

BOOST_AUTO_TEST_SUITE_END()
