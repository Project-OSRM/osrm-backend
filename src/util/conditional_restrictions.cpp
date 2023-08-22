#include "util/conditional_restrictions.hpp"

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

namespace osrm::util
{

#ifndef NDEBUG
// Debug output stream operators for use with BOOST_SPIRIT_DEBUG
inline std::ostream &operator<<(std::ostream &stream, const ConditionalRestriction &restriction)
{
    return stream << restriction.value << "=" << restriction.condition;
}
#endif
} // namespace osrm::util

BOOST_FUSION_ADAPT_STRUCT(osrm::util::ConditionalRestriction,
                          (std::string, value)(std::string, condition))

namespace osrm::util
{
namespace detail
{

namespace
{
namespace qi = boost::spirit::qi;
} // namespace

template <typename Iterator, typename Skipper = qi::blank_type>
struct conditional_restrictions_grammar
    : qi::grammar<Iterator, Skipper, std::vector<ConditionalRestriction>()>
{
    // http://wiki.openstreetmap.org/wiki/Conditional_restrictions
    conditional_restrictions_grammar() : conditional_restrictions_grammar::base_type(restrictions)
    {
        using qi::_1;
        using qi::_val;
        using qi::lit;

        // clang-format off

        restrictions
            = restriction % ';'
            ;

        restriction
            = value >> '@' >> condition
            ;

        value
            = +(qi::char_ - '@')
            ;

        condition
            = *qi::blank
            >> (lit('(') >> qi::as_string[qi::no_skip[*~lit(')')]][_val = _1] >> lit(')')
                | qi::as_string[qi::no_skip[*~lit(';')]][_val = _1]
               )
            ;

        // clang-format on

        BOOST_SPIRIT_DEBUG_NODES((restrictions)(restriction)(value)(condition));
    }

    qi::rule<Iterator, Skipper, std::vector<ConditionalRestriction>()> restrictions;
    qi::rule<Iterator, Skipper, ConditionalRestriction()> restriction;
    qi::rule<Iterator, Skipper, std::string()> value, condition;
};
} // namespace detail

std::vector<ConditionalRestriction> ParseConditionalRestrictions(const std::string &str)
{
    auto it(str.begin()), end(str.end());
    const detail::conditional_restrictions_grammar<decltype(it)> static grammar;

    std::vector<ConditionalRestriction> result;
    bool ok = boost::spirit::qi::phrase_parse(it, end, grammar, boost::spirit::qi::blank, result);

    if (!ok || it != end)
        return std::vector<ConditionalRestriction>();

    return result;
}

} // namespace osrm::util
