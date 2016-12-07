#include "util/opening_hours.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(opening_hours)

// convert a string representation of time to a tm structure
struct tm time(const char *str)
{
    const std::locale loc = std::locale(
        std::locale::classic(), new boost::posix_time::time_input_facet("%a, %d %b %Y %H:%M:%S"));
    std::istringstream is(str);
    is.imbue(loc);

    boost::posix_time::ptime t;
    is >> t;
    return boost::posix_time::to_tm(t);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_grammar)
{
    using osrm::util::ParseOpeningHours;

    const std::string opening_hours[] = {
        "Apr 10-Jun 15",
        "Apr 10-15 off",
        "Jun 08:00-14:00",
        "24/7",
        "Mo-Sa",
        "Sa-Su 00:00-24:00",
        "Mo-Fr 08:30-20:00",
        "Mo 10:00-12:00,12:30-15:00; Tu-Fr 08:00-12:00,12:30-15:00; Sa 08:00-12:00",
        "Mo-Su 08:00-18:00; Apr 10-15 off; Jun 08:00-14:00; Aug off; Dec 25 off",
        "Mo-Sa 10:00-20:00; Tu off",
        "Mo-Sa 10:00-20:00; Tu 10:00-14:00",
        "sunrise-(sunset-01:30)",
        "Su 10:00+",
        "Mo-Sa 08:00-13:00,14:00-17:00 || \"by appointment\"",
        "Su-Tu 11:00-01:00, We-Th 11:00-03:00, Fr 11:00-06:00, Sa 11:00-07:00",
        "week 01-53/2 Fr 09:00-12:00; week 02-52/2 We 09:00-12:00",
        "Mo-Su,PH 15:00-03:00; easter -2 days off",
        "08:30-12:30,15:30-20:00",
        "Tu,Th 16:00-20:00",
        "2016 Feb-2017 Dec",
        "2016-2017",
        "Mo,Tu,Th,Fr 12:00-18:00;Sa 12:00-17:00; Th[3] off; Th[-1] off"};

    for (auto &input : opening_hours)
    {
        BOOST_CHECK_MESSAGE(!ParseOpeningHours(input).empty(), "parsing " << input << " failed");
    }
}

BOOST_AUTO_TEST_CASE(check_opening_hours_grammar_incorrect_correct)
{
    using osrm::util::ParseOpeningHours;

    const std::pair<std::string, std::string> opening_hours[] = {
        {"7/8-23", "Mo-Su 08:00-23:00"},
        {"0600-1800", "06:00-18:00"},
        {"Mo-Fr, 07:00-09:00", "Mo-Fr 07:00-09:00"},
        {"07;00-2;00pm", "07:00-14:00"},
        {"08.00-16.00, public room till 03.00 a.m",
         "08:00-16:00 open, 16:00-03:00 off \"public room\""},
        {"09:00-21:00 TEL/072(360)3200", "09:00-21:00 \"call us\""},
        {"10:00 - 13:30 / 17:00 - 20:30", "10:00-13:30,17:00-20:30"},
        {"April-September; Mo-Fr 09:00-13:00, 14:00-18:00, Sa 10:00-13:00",
         "Apr-Sep: Mo-Fr 09:00-13:00,14:00-18:00; Apr-Sep: Sa 10:00-13:00"},
        {"Dining in: 6am to 11pm; Drive thru: 24/7",
         "06:00-23:00 open \"Dining in\" || 00:00-24:00 open \"Drive-through\""},
        {"MWThF: 1200-1800; SaSu: 1200-1700", "Mo,We,Th,Fr 12:00-18:00; Sa-Su 12:00-17:00"},
        {"BAR: Su-Mo 18:00-02:00; Tu-Th 18:00-03:00; Fr-Sa 18:00-04:00; CLUB: Tu-Th 20:00-03:00; "
         "Fr-Sa 20:00-04:00",
         "Tu-Th 20:00-03:00 open \"Club and bar\"; Fr-Sa 20:00-04:00 open \"Club and bar\" || "
         "Su-Mo 18:00-02:00 open \"bar\" || Tu-Th 18:00-03:00 open \"bar\" || Fr-Sa 18:00-04:00 "
         "open \"bar\""}};

    for (auto &input : opening_hours)
    {
        BOOST_CHECK_MESSAGE(ParseOpeningHours(input.first).empty(),
                            "parsing succeed for incorrect input" << input.first);
        BOOST_CHECK_MESSAGE(!ParseOpeningHours(input.second).empty(),
                            "parsing " << input.second << " failed");
    }
}

BOOST_AUTO_TEST_CASE(check_rules_grouping)
{
    using osrm::util::ParseOpeningHours;

    const auto &result1 = ParseOpeningHours("Su-Th 11:00-03:00, Fr-Sa 11:00-05:00");
    BOOST_REQUIRE_EQUAL(result1.size(), 2);
    BOOST_CHECK_EQUAL(result1.at(0).times.size(), 1);
    BOOST_CHECK_EQUAL(result1.at(0).weekdays.size(), 1);
    BOOST_CHECK_EQUAL(result1.at(0).monthdays.size(), 0);
    BOOST_CHECK_EQUAL(result1.at(1).times.size(), 1);
    BOOST_CHECK_EQUAL(result1.at(1).weekdays.size(), 1);
    BOOST_CHECK_EQUAL(result1.at(1).monthdays.size(), 0);

    const auto &result2 = ParseOpeningHours("Su-Th,Fr-Sa 11:00-12:00,14:00-05:00");
    BOOST_REQUIRE_EQUAL(result2.size(), 1);
    BOOST_CHECK_EQUAL(result2.at(0).times.size(), 2);
    BOOST_CHECK_EQUAL(result2.at(0).weekdays.size(), 2);
    BOOST_CHECK_EQUAL(result2.at(0).monthdays.size(), 0);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_time_and_weekday)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("Mo-Fr 08:30-20:00");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 14 Dec 2016 12:32:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 14 Dec 2016 21:32:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sat, 17 Dec 2016 12:32:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sun, 18 Dec 2016 12:32:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 19 Dec 2016 08:30:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 19 Dec 2016 08:29:59")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Fri, 23 Dec 2016 19:59:59")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Fri, 23 Dec 2016 20:00:00")), false);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_year_month)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("2016 Feb-2017 Dec");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sun, 31 Jan 2016 12:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 14 Dec 2016 12:32:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sun, 31 Dec 2017 12:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 01 Jan 2018 12:00:00")), false);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_year_monthday)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("2019 Apr 10-Jun 15");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sun, 16 Apr 2017 17:59:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 09 Apr 2019 23:59:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 16 Apr 2019 00:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sat, 15 Jun 2019 00:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sun, 16 Jun 2019 00:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Thu, 16 Apr 2020 00:00:00")), false);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_year_monthday_time_and_weekday)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("2017 Feb-May Sa-Tu 08:00-18:00");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 12:32:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 31 Jan 2017 23:59:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 01 Feb 2017 08:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sat, 04 Feb 2017 09:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 07 Feb 2017 09:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 08 Feb 2017 09:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sun, 16 Apr 2017 17:59:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Thu, 01 Jun 2017 00:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sun, 04 Mar 2018 12:32:00")), false);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_time_plus)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("08:00+");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 07:59:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 08:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 23:59:00")), true);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_off)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("08:00-20:00; 12:00-14:30 off; 12:30-13:30 open");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 07:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 08:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 12:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 13:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 13:30:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 14:30:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 20:00:00")), false);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_overnight_multiple_times)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("20 08:00-10:00,20:00-03:00");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 02:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 07:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 09:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 12:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 21:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 21 Dec 2016 02:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 21 Dec 2016 07:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 21 Dec 2016 09:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 21 Dec 2016 21:00:00")), false);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_overnight_weekdays)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("Mo-Fr 20:00-03:00");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 02:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sat, 17 Dec 2016 02:00:00")), true);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_overnight_days)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("Dec 12-17 20:00-03:00");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 02:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 12 Dec 2016 20:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sat, 17 Dec 2016 02:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sat, 17 Dec 2016 20:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sun, 18 Dec 2016 02:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Sun, 18 Dec 2016 20:00:00")), false);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_extended_hours_overlapping)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("Dec 20 08:00-44:00");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 19 Dec 2016 07:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 07:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 21 Dec 2016 07:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Thu, 22 Dec 2016 07:00:00")), false);

    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 19 Dec 2016 08:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 08:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 21 Dec 2016 08:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Thu, 22 Dec 2016 08:00:00")), false);

    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 19 Dec 2016 20:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 20:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 21 Dec 2016 20:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Thu, 22 Dec 2016 20:00:00")), false);
}

BOOST_AUTO_TEST_CASE(check_opening_hours_extended_hours_nonoverlapping)
{
    using osrm::util::ParseOpeningHours;
    using osrm::util::CheckOpeningHours;

    const auto &opening_hours = ParseOpeningHours("Dec 20 20:00-32:00");
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 19 Dec 2016 07:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 07:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 21 Dec 2016 07:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Thu, 22 Dec 2016 07:00:00")), false);

    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 19 Dec 2016 08:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 08:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 21 Dec 2016 08:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Thu, 22 Dec 2016 08:00:00")), false);

    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Mon, 19 Dec 2016 20:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Tue, 20 Dec 2016 20:00:00")), true);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Wed, 21 Dec 2016 20:00:00")), false);
    BOOST_CHECK_EQUAL(CheckOpeningHours(opening_hours, time("Thu, 22 Dec 2016 20:00:00")), false);
}

BOOST_AUTO_TEST_SUITE_END()
