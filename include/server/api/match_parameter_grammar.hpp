#ifndef MATCH_PARAMETERS_GRAMMAR_HPP
#define MATCH_PARAMETERS_GRAMMAR_HPP

#include "server/api/route_parameters_grammar.hpp"
#include "engine/api/match_parameters.hpp"

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

namespace osrm
{
namespace server
{
namespace api
{

namespace
{
namespace ph = boost::phoenix;
namespace qi = boost::spirit::qi;
}

template <typename Iterator = std::string::iterator,
          typename Signature = void(engine::api::MatchParameters &)>
struct MatchParametersGrammar final : public RouteParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = RouteParametersGrammar<Iterator, Signature>;

    MatchParametersGrammar() : BaseGrammar(root_rule)
    {
#ifdef BOOST_HAS_LONG_LONG
        if (std::is_same<std::size_t, unsigned long long>::value)
            size_t_ = qi::ulong_long;
        else
            size_t_ = qi::ulong_;
#else
        size_t_ = qi::ulong_;
#endif

        timestamps_rule =
            qi::lit("timestamps=") >
            (qi::uint_ %
             ';')[ph::bind(&engine::api::MatchParameters::timestamps, qi::_r1) = qi::_1];

        gaps_type.add("split", engine::api::MatchParameters::GapsType::Split)(
            "ignore", engine::api::MatchParameters::GapsType::Ignore);

        root_rule =
            BaseGrammar::query_rule(qi::_r1) > -qi::lit(".json") >
            -('?' > (timestamps_rule(qi::_r1) | BaseGrammar::base_rule(qi::_r1) |
                     (qi::lit("gaps=") >
                      gaps_type[ph::bind(&engine::api::MatchParameters::gaps, qi::_r1) = qi::_1]) |
                     (qi::lit("tidy=") >
                      qi::bool_[ph::bind(&engine::api::MatchParameters::tidy, qi::_r1) = qi::_1])) %
                        '&');
    }

  private:
    qi::rule<Iterator, Signature> root_rule;
    qi::rule<Iterator, Signature> timestamps_rule;
    qi::rule<Iterator, std::size_t()> size_t_;

    qi::symbols<char, engine::api::MatchParameters::GapsType> gaps_type;
};
}
}
}

#endif
