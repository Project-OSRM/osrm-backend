#ifndef TABLE_PARAMETERS_GRAMMAR_HPP
#define TABLE_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"
#include "engine/api/table_parameters.hpp"

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
          typename Signature = void(engine::api::TableParameters &)>
struct TableParametersGrammar final : public BaseParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = BaseParametersGrammar<Iterator, Signature>;

    TableParametersGrammar() : BaseGrammar(root_rule)
    {
#ifdef BOOST_HAS_LONG_LONG
        if (std::is_same<std::size_t, unsigned long long>::value)
            size_t_ = qi::ulong_long;
        else
            size_t_ = qi::ulong_;
#else
        size_t_ = qi::ulong_;
#endif

        destinations_rule =
            qi::lit("destinations=") >
            (qi::lit("all") |
             (size_t_ %
              ';')[ph::bind(&engine::api::TableParameters::destinations, qi::_r1) = qi::_1]);

        sources_rule =
            qi::lit("sources=") >
            (qi::lit("all") |
             (size_t_ % ';')[ph::bind(&engine::api::TableParameters::sources, qi::_r1) = qi::_1]);

        table_rule = destinations_rule(qi::_r1) | sources_rule(qi::_r1);

        root_rule = BaseGrammar::query_rule(qi::_r1) > -qi::lit(".json") >
                    -('?' > (table_rule(qi::_r1) | BaseGrammar::base_rule(qi::_r1)) % '&');
    }

  private:
    qi::rule<Iterator, Signature> root_rule;
    qi::rule<Iterator, Signature> table_rule;
    qi::rule<Iterator, Signature> sources_rule;
    qi::rule<Iterator, Signature> destinations_rule;
    qi::rule<Iterator, std::size_t()> size_t_;
};
}
}
}

#endif
