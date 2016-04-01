#ifndef TABLE_PARAMETERS_GRAMMAR_HPP
#define TABLE_PARAMETERS_GRAMMAR_HPP

#include "engine/api/table_parameters.hpp"

#include "server/api/base_parameters_grammar.hpp"

#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/spirit/include/qi_grammar.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_optional.hpp>

namespace osrm
{
namespace server
{
namespace api
{

namespace qi = boost::spirit::qi;

struct TableParametersGrammar final : public BaseParametersGrammar
{
    using Iterator = std::string::iterator;
    using SourcesT = std::vector<std::size_t>;
    using DestinationsT = std::vector<std::size_t>;

    TableParametersGrammar() : BaseParametersGrammar(root_rule, parameters)
    {
        const auto set_destiantions = [this](DestinationsT dests)
        {
            parameters.destinations = std::move(dests);
        };
        const auto set_sources = [this](SourcesT sources)
        {
            parameters.sources = std::move(sources);
        };
        destinations_rule = (qi::lit("destinations=") >> (qi::ulong_ % ";")[set_destiantions]) |
                            qi::lit("destinations=all");
        sources_rule =
            (qi::lit("sources=") >> (qi::ulong_ % ";")[set_sources]) | qi::lit("sources=all");
        table_rule = destinations_rule | sources_rule;

        root_rule =
            query_rule >> -qi::lit(".json") >> -(qi::lit("?") >> (table_rule | base_rule) % '&');
    }

    engine::api::TableParameters parameters;

  private:
    qi::rule<Iterator> root_rule;
    qi::rule<Iterator> table_rule;
    qi::rule<Iterator> sources_rule;
    qi::rule<Iterator> destinations_rule;
};
}
}
}

#endif
