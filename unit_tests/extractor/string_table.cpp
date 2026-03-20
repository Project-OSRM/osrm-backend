#include "extractor/string_table.hpp"
#include "util/exception.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <iomanip>
#include <typeinfo>
#include <vector>

BOOST_AUTO_TEST_SUITE(string_table)

using namespace osrm;
using namespace osrm::extractor;

StringTable::IndexedData PrepareStringTableData(std::vector<std::string> &data, bool fill_all)
{
    StringTable::IndexedData indexed_data;
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

    return StringTable::IndexedData(name_offsets.begin(), name_offsets.end(), name_char_data.begin());
}

BOOST_AUTO_TEST_CASE(check_string_table_fill)
{
    std::vector<std::string> expected_names = {
        "",     "A", "check_name", "ccc", "dDDd", "E", "ff", "ggg", "hhhh", "I", "jj", "",  "kkk",
        "llll", "M", "nn",         "ooo", "pppp", "q", "r",  "S",   "T",    "",  "u",  "V", "W",
        "X",    "Y", "Z",          "",    "",     "",  "",   "",    "",     "",  "0",  ""};

    auto data = PrepareStringTableData(expected_names, true);
    StringTable string_table{data};

    for (std::size_t index = 0; index < expected_names.size(); ++index)
    {
        const StringViewID id = 5 * index;
        BOOST_CHECK_EQUAL(string_table.GetNameForID(id), expected_names[index]);
        BOOST_CHECK_EQUAL(string_table.GetRefForID(id), expected_names[index] + "_ref");
        BOOST_CHECK_EQUAL(string_table.GetDestinationsForID(id), expected_names[index] + "_des");
        BOOST_CHECK_EQUAL(string_table.GetPronunciationForID(id), expected_names[index] + "_pro");
        BOOST_CHECK_EQUAL(string_table.GetExitsForID(id), expected_names[index] + "_ext");
    }
}

BOOST_AUTO_TEST_CASE(check_string_table_nofill)
{
    std::vector<std::string> expected_names = {
        "",     "A", "check_name", "ccc", "dDDd", "E", "ff", "ggg", "hhhh", "I", "jj", "",  "kkk",
        "llll", "M", "nn",         "ooo", "pppp", "q", "r",  "S",   "T",    "",  "u",  "V", "W",
        "X",    "Y", "Z",          "",    "",     "",  "",   "",    "",     "",  "0",  ""};

    auto data = PrepareStringTableData(expected_names, false);
    StringTable string_table{data};

    // CALLGRIND_START_INSTRUMENTATION;
    for (std::size_t index = 0; index < expected_names.size(); ++index)
    {
        const StringViewID id = 5 * index;
        BOOST_CHECK_EQUAL(string_table.GetNameForID(id), expected_names[index]);
        BOOST_CHECK(string_table.GetRefForID(id).empty());
        BOOST_CHECK(string_table.GetDestinationsForID(id).empty());
        BOOST_CHECK(string_table.GetPronunciationForID(id).empty());
        BOOST_CHECK(string_table.GetExitsForID(id).empty());
    }
    // CALLGRIND_STOP_INSTRUMENTATION;
}

BOOST_AUTO_TEST_CASE(check_invalid_ids)
{
    StringTable string_table;
    BOOST_CHECK_EQUAL(string_table.GetNameForID(INVALID_STRINGVIEWID), "");
    BOOST_CHECK_EQUAL(string_table.GetRefForID(INVALID_STRINGVIEWID), "");
    BOOST_CHECK_EQUAL(string_table.GetDestinationsForID(INVALID_STRINGVIEWID), "");
    BOOST_CHECK_EQUAL(string_table.GetPronunciationForID(INVALID_STRINGVIEWID), "");
    BOOST_CHECK_EQUAL(string_table.GetExitsForID(INVALID_STRINGVIEWID), "");
}

BOOST_AUTO_TEST_SUITE_END()
