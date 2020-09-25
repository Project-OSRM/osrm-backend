#ifndef NEAREST_PARAMETERS_GRAMMAR_HPP
#define NEAREST_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"
#include "engine/api/nearest_parameters.hpp"

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
          typename Signature = void(engine::api::NearestParameters &)>
struct NearestParametersGrammar final : public BaseParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = BaseParametersGrammar<Iterator, Signature>;

    NearestParametersGrammar() : BaseGrammar(root_rule)
    {
        nearest_rule = (qi::lit("number=") >
                        qi::uint_)[ph::bind(&engine::api::NearestParameters::number_of_results,
                                            qi::_r1) = qi::_1];

        root_rule = BaseGrammar::query_rule(qi::_r1) > BaseGrammar::format_rule(qi::_r1) >
                    -('?' > (nearest_rule(qi::_r1) | BaseGrammar::base_rule(qi::_r1)) % '&');
    }

  private:
    qi::rule<Iterator, Signature> root_rule;
    qi::rule<Iterator, Signature> nearest_rule;
};
}
}
}

#endif
