#ifndef ISO_8601_DURATION_PARSER_HPP
#define ISO_8601_DURATION_PARSER_HPP

#include <boost/bind.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_action.hpp>

namespace osrm
{
namespace util
{

namespace qi = boost::spirit::qi;

template <typename Iterator> struct iso_8601_grammar : qi::grammar<Iterator>
{
    iso_8601_grammar()
        : iso_8601_grammar::base_type(iso_period), temp(0), hours(0), minutes(0), seconds(0)
    {
        iso_period = qi::lit('P') >> qi::lit('T') >>
                     ((value >> hour >> value >> minute >> value >> second) |
                      (value >> hour >> value >> minute) | (value >> hour >> value >> second) |
                      (value >> hour) | (value >> minute >> value >> second) | (value >> minute) |
                      (value >> second));

        value = qi::uint_[boost::bind(&iso_8601_grammar<Iterator>::set_temp, this, ::_1)];
        second = (qi::lit('s') |
                  qi::lit('S'))[boost::bind(&iso_8601_grammar<Iterator>::set_seconds, this)];
        minute = (qi::lit('m') |
                  qi::lit('M'))[boost::bind(&iso_8601_grammar<Iterator>::set_minutes, this)];
        hour = (qi::lit('h') |
                qi::lit('H'))[boost::bind(&iso_8601_grammar<Iterator>::set_hours, this)];
    }

    qi::rule<Iterator> iso_period;
    qi::rule<Iterator, std::string()> value, hour, minute, second;

    unsigned temp;
    unsigned hours;
    unsigned minutes;
    unsigned seconds;

    void set_temp(unsigned number) { temp = number; }

    void set_hours()
    {
        if (temp < 24)
        {
            hours = temp;
        }
    }

    void set_minutes()
    {
        if (temp < 60)
        {
            minutes = temp;
        }
    }

    void set_seconds()
    {
        if (temp < 60)
        {
            seconds = temp;
        }
    }

    unsigned get_duration() const
    {
        unsigned temp = (3600 * hours + 60 * minutes + seconds);
        if (temp == 0)
        {
            temp = std::numeric_limits<unsigned>::max();
        }
        return temp;
    }
};
}
}

#endif // ISO_8601_DURATION_PARSER_HPP
