#include "util/opening_hours.hpp"

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

#include <boost/io/ios_state.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iterator>

namespace osrm
{
namespace util
{

#ifndef NDEBUG
// Debug output stream operators for use with BOOST_SPIRIT_DEBUG
inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::Modifier value)
{
    switch (value)
    {
    case OpeningHours::unknown:
        return stream << "unknown";
    case OpeningHours::open:
        return stream << "open";
    case OpeningHours::closed:
        return stream << "closed";
    case OpeningHours::off:
        return stream << "off";
    case OpeningHours::is24_7:
        return stream << "24/7";
    }
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::Time::Event value)
{
    switch (value)
    {
    case OpeningHours::Time::dawn:
        return stream << "dawn";
    case OpeningHours::Time::sunrise:
        return stream << "sunrise";
    case OpeningHours::Time::sunset:
        return stream << "sunset";
    case OpeningHours::Time::dusk:
        return stream << "dusk";
    default:
        break;
    }
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::Time &value)
{
    boost::io::ios_flags_saver ifs(stream);
    if (value.event == OpeningHours::Time::invalid)
        return stream << "???";
    if (value.event == OpeningHours::Time::none)
        return stream << std::setfill('0') << std::setw(2) << value.minutes / 60 << ":"
                      << std::setfill('0') << std::setw(2) << value.minutes % 60;
    stream << value.event;
    if (value.minutes != 0)
        stream << value.minutes;
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::TimeSpan &value)
{
    return stream << value.from << "-" << value.to;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::Monthday &value)
{
    bool empty = true;
    if (value.year != 0)
    {
        stream << (int)value.year;
        empty = false;
    };
    if (value.month != 0)
    {
        stream << (empty ? "" : "/") << (int)value.month;
        empty = false;
    };
    if (value.day != 0)
    {
        stream << (empty ? "" : "/") << (int)value.day;
    };
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::WeekdayRange &value)
{
    boost::io::ios_flags_saver ifs(stream);
    return stream << std::hex << std::setfill('0') << std::setw(2) << value.weekdays;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::MonthdayRange &value)
{
    return stream << value.from << "-" << value.to;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours &value)
{
    if (value.modifier == OpeningHours::is24_7)
        return stream << OpeningHours::is24_7;

    for (auto x : value.monthdays)
        stream << x << ", ";
    for (auto x : value.weekdays)
        stream << x << ", ";
    for (auto x : value.times)
        stream << x << ", ";
    return stream << " |" << value.modifier << "|";
}
#endif

namespace detail
{

namespace
{
namespace ph = boost::phoenix;
namespace qi = boost::spirit::qi;
} // namespace

template <typename Iterator, typename Skipper = qi::blank_type>
struct opening_hours_grammar : qi::grammar<Iterator, Skipper, std::vector<OpeningHours>()>
{
    // http://wiki.openstreetmap.org/wiki/Key:opening_hours/specification
    opening_hours_grammar() : opening_hours_grammar::base_type(time_domain)
    {
        using qi::_1;
        using qi::_a;
        using qi::_b;
        using qi::_c;
        using qi::_pass;
        using qi::_r1;
        using qi::_val;
        using qi::char_;
        using qi::eoi;
        using qi::lit;
        using qi::uint_;
        using oh = osrm::util::OpeningHours;

        // clang-format off

        // General syntax
        time_domain = rule_sequence[ph::push_back(_val, _1)] % any_rule_separator;

        rule_sequence
            = lit("24/7")[ph::bind(&oh::modifier, _val) = oh::is24_7]
            | (selector_sequence[_val = _1] >> -rule_modifier[ph::bind(&oh::modifier, _val) = _1] >> -comment)
            | comment
            ;

        any_rule_separator = char_(';') | lit("||") | additional_rule_separator;

        additional_rule_separator = char_(',');

        // Rule modifiers
        rule_modifier.add
            ("unknown", oh::unknown)
            ("open", oh::open)
            ("closed", oh::closed)
            ("off", oh::off)
            ;

        // Selectors
        selector_sequence = (wide_range_selectors(_a) >> small_range_selectors(_a))[_val = _a];

        wide_range_selectors
            = (-monthday_selector(_r1)
               >> -year_selector(_r1)
               >> -week_selector(_r1) // TODO week_selector
              ) >> -lit(':')
            ;

        small_range_selectors = -(weekday_selector(_r1) >> (&~lit(',') | eoi)) >> -time_selector(_r1);

        // Time selector
        time_selector = (timespan % ',')[ph::bind(&OpeningHours::times, _r1) = _1];

        timespan
            = (time[_a = _1]
               >> -(lit('+')[_b = ph::construct<OpeningHours::Time>(24, 0)]
                    | ('-' >> extended_time[_b = _1]
                       >> -('+' | '/' >> (minute | hour_minutes))))
               )[_val = ph::construct<OpeningHours::TimeSpan>(_a, _b)]
            ;

        time = hour_minutes | variable_time;

        extended_time = extended_hour_minutes | variable_time;

        variable_time
            = event[_val = ph::construct<OpeningHours::Time>(_1)]
            | ('(' >> event[_a = _1] >> plus_or_minus[_b = _1] >> hour_minutes[_c = _1] >> ')')
            [_val = ph::construct<OpeningHours::Time>(_a, _b, _c)]
            ;

        event.add
            ("dawn", OpeningHours::Time::dawn)
            ("sunrise", OpeningHours::Time::sunrise)
            ("sunset", OpeningHours::Time::sunset)
            ("dusk", OpeningHours::Time::dusk)
            ;

        // Weekday selector
        weekday_selector
            = (holiday_sequence(_r1) >> -(char_(", ") >> weekday_sequence(_r1)))
            | (weekday_sequence(_r1) >> -(char_(", ") >> holiday_sequence(_r1)))
            ;

        weekday_sequence = (weekday_range % ',')[ph::bind(&OpeningHours::weekdays, _r1) = _1];

        weekday_range
            = (wday[_a = _1, _b = _1]
               >> -(('-' >> wday[_b = _1])
                    | ('[' >> (nth_entry % ',') >> ']' >> -day_offset)))
            [_val = ph::construct<OpeningHours::WeekdayRange>(_a, _b)]
            ;

        holiday_sequence = (lit("SH") >> -day_offset) | lit("PH");

        nth_entry = nth | nth >> '-' >> nth | '-' >> nth;

        nth = char_("12345");

        day_offset = plus_or_minus >> uint_ >> lit("days");

        // Week selector
        week_selector = (lit("week ") >> week) % ',';

        week = weeknum >> -('-' >> weeknum >> -('/' >> uint_));

        // Month selector
        monthday_selector = (monthday_range % ',')[ph::bind(&OpeningHours::monthdays, _r1) = _1];

        monthday_range
            = (date_from[ph::bind(&OpeningHours::MonthdayRange::from, _val) = _1]
               >> -date_offset
               >> '-'
               >> date_to[ph::bind(&OpeningHours::MonthdayRange::to, _val) = _1]
               >> -date_offset)
            | (date_from[ph::bind(&OpeningHours::MonthdayRange::from, _val) = _1]
               >> -(date_offset
                    >> -lit('+')[ph::bind(&OpeningHours::MonthdayRange::from, _val) = ph::construct<OpeningHours::Monthday>(-1)]
                    ))
            ;

        date_offset = (plus_or_minus >> wday) | day_offset;

        date_from
            = ((-year[_a = _1] >> ((month[_b = _1] >> -daynum[_c = _1]) | daynum[_c = _1]))
               | variable_date)
            [_val = ph::construct<OpeningHours::Monthday>(_a, _b, _c)]
            ;

        date_to
            = date_from[_val = _1]
            | daynum[_val = ph::construct<OpeningHours::Monthday>(0, 0, _1)]
            ;

        variable_date = lit("easter");

        // Year selector
        year_selector = (year_range % ',')[ph::bind(&OpeningHours::monthdays, _r1) = _1];

        year_range
            = year[ph::bind(&OpeningHours::MonthdayRange::from, _val) = ph::construct<OpeningHours::Monthday>(_1)]
            >> -(('-' >> year[ph::bind(&OpeningHours::MonthdayRange::to, _val) = ph::construct<OpeningHours::Monthday>(_1)]
                  >> -('/' >> uint_))
                 | lit('+')[ph::bind(&OpeningHours::MonthdayRange::to, _val) = ph::construct<OpeningHours::Monthday>(-1)]);

        // Basic elements
        plus_or_minus = lit('+')[_val = true] | lit('-')[_val = false];

        hour = uint2_p[_pass = bind([](unsigned x) { return x <= 24; }, _1), _val = _1];

        extended_hour = uint2_p[_pass = bind([](unsigned x) { return x <= 48; }, _1), _val = _1];

        minute = uint2_p[_pass = bind([](unsigned x) { return x < 60; }, _1), _val = _1];

        hour_minutes =
            hour[_a = _1] >> ':' >> minute[_val = ph::construct<OpeningHours::Time>(_a, _1)];

        extended_hour_minutes = extended_hour[_a = _1] >> ':' >>
                                minute[_val = ph::construct<OpeningHours::Time>(_a, _1)];

        wday.add
            ("Su", 0)
            ("Mo", 1)
            ("Tu", 2)
            ("We", 3)
            ("Th", 4)
            ("Fr", 5)
            ("Sa", 6)
            ;

        daynum
            = uint2_p[_pass = bind([](unsigned x) { return 01 <= x && x <= 31; }, _1), _val = _1]
            >> !qi::no_skip[lit(':') >> uint2_p] // distinguish "daynum:.." from "hour:minute"
            ;

        weeknum = uint2_p[_pass = bind([](unsigned x) { return 01 <= x && x <= 53; }, _1), _val = _1];

        month.add
            ("Jan", 1)
            ("Feb", 2)
            ("Mar", 3)
            ("Apr", 4)
            ("May", 5)
            ("Jun", 6)
            ("Jul", 7)
            ("Aug", 8)
            ("Sep", 9)
            ("Oct", 10)
            ("Nov", 11)
            ("Dec", 12)
            ;

        year = uint4_p[_pass = bind([](unsigned x) { return x > 1900; }, _1), _val = _1];

        comment = lit('"') >> *(~qi::char_('"')) >> lit('"');

        // clang-format on

        BOOST_SPIRIT_DEBUG_NODES((time_domain)(rule_sequence)(any_rule_separator)(
            selector_sequence)(wide_range_selectors)(small_range_selectors)(time_selector)(
            timespan)(time)(extended_time)(variable_time)(weekday_selector)(weekday_sequence)(
            weekday_range)(holiday_sequence)(nth_entry)(nth)(day_offset)(week_selector)(week)(
            monthday_selector)(monthday_range)(date_offset)(date_from)(date_to)(variable_date)(
            year_selector)(year_range)(plus_or_minus)(hour_minutes)(extended_hour_minutes)(comment)(
            hour)(extended_hour)(minute)(daynum)(weeknum)(year));
    }

    qi::rule<Iterator, Skipper, std::vector<OpeningHours>()> time_domain;
    qi::rule<Iterator, Skipper, OpeningHours()> rule_sequence;
    qi::rule<Iterator, Skipper, void()> any_rule_separator, additional_rule_separator;
    qi::rule<Iterator, Skipper, OpeningHours(), qi::locals<OpeningHours>> selector_sequence;
    qi::symbols<char const, OpeningHours::Modifier> rule_modifier;
    qi::rule<Iterator, Skipper, void(OpeningHours &)> wide_range_selectors, small_range_selectors,
        time_selector, weekday_selector, year_selector, monthday_selector, week_selector;

    // Time rules
    qi::rule<Iterator,
             Skipper,
             OpeningHours::TimeSpan(),
             qi::locals<OpeningHours::Time, OpeningHours::Time>>
        timespan;

    qi::rule<Iterator, Skipper, OpeningHours::Time()> time, extended_time;

    qi::rule<Iterator,
             Skipper,
             OpeningHours::Time(),
             qi::locals<OpeningHours::Time::Event, bool, OpeningHours::Time>>
        variable_time;

    qi::rule<Iterator, Skipper, OpeningHours::Time(), qi::locals<unsigned>> hour_minutes,
        extended_hour_minutes;

    qi::symbols<char const, OpeningHours::Time::Event> event;

    qi::rule<Iterator, Skipper, bool()> plus_or_minus;

    // Weekday rules
    qi::rule<Iterator, Skipper, void(OpeningHours &)> weekday_sequence, holiday_sequence;

    qi::rule<Iterator,
             Skipper,
             OpeningHours::WeekdayRange(),
             qi::locals<unsigned char, unsigned char>>
        weekday_range;

    // Monthday rules
    qi::rule<Iterator, Skipper, OpeningHours::MonthdayRange()> monthday_range;

    qi::rule<Iterator, Skipper, OpeningHours::Monthday(), qi::locals<unsigned, unsigned, unsigned>>
        date_from;

    qi::rule<Iterator, Skipper, OpeningHours::Monthday()> date_to;

    // Year rules
    qi::rule<Iterator, Skipper, OpeningHours::MonthdayRange()> year_range;

    // Unused rules
    qi::rule<Iterator, Skipper, void()> nth_entry, nth, day_offset, week, date_offset,
        variable_date, comment;

    // Basic rules and parsers
    qi::rule<Iterator, Skipper, unsigned()> hour, extended_hour, minute, daynum, weeknum, year;
    qi::symbols<char const, unsigned char> wday, month;
    qi::uint_parser<unsigned, 10, 2, 2> uint2_p;
    qi::uint_parser<unsigned, 10, 4, 4> uint4_p;
};
} // namespace detail

std::vector<OpeningHours> ParseOpeningHours(const std::string &str)
{
    auto it(str.begin()), end(str.end());
    const detail::opening_hours_grammar<decltype(it)> static grammar;

    std::vector<OpeningHours> result;
    bool ok = boost::spirit::qi::phrase_parse(it, end, grammar, boost::spirit::qi::blank, result);

    if (!ok || it != end)
        return std::vector<OpeningHours>();

    return result;
}

bool CheckOpeningHours(const std::vector<OpeningHours> &input, const struct tm &time)
{
    bool is_open = false;
    for (auto &opening_hours : input)
    {
        if (opening_hours.modifier == OpeningHours::is24_7)
            return true;

        if (opening_hours.IsInRange(time))
        {
            is_open = opening_hours.modifier == OpeningHours::open;
        }
    }

    return is_open;
}

} // namespace util
} // namespace osrm
