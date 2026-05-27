#include "util/conditional_restrictions.hpp"

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>

BOOST_FUSION_ADAPT_STRUCT(osrm::util::ConditionalRestriction,
                          (std::string, value)(std::string, condition))

namespace osrm::util
{
namespace detail
{

namespace x3 = boost::spirit::x3;

// clang-format off

const auto value = x3::rule<struct value_tag, std::string>{} =
    +(x3::char_ - '@');

const auto condition = x3::rule<struct condition_tag, std::string>{} =
    *x3::blank >>
    (x3::lit('(') >> x3::raw[*~x3::char_(')')] >> x3::lit(')')
     | x3::raw[*~x3::char_(';')]);

const auto restriction = x3::rule<struct restriction_tag, ConditionalRestriction>{} =
    value >> '@' >> condition;

const auto restrictions = x3::rule<struct restrictions_tag, std::vector<ConditionalRestriction>>{} =
    restriction % ';';

// clang-format on

} // namespace detail

std::vector<ConditionalRestriction> ParseConditionalRestrictions(const std::string &str)
{
    namespace x3 = boost::spirit::x3;
    auto it(str.begin()), end(str.end());

    std::vector<ConditionalRestriction> result;
    bool ok = x3::phrase_parse(it, end, detail::restrictions, x3::blank, result);

    if (!ok || it != end)
        return std::vector<ConditionalRestriction>();

    return result;
}

} // namespace osrm::util
