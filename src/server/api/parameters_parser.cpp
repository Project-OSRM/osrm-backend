#include "server/api/parameters_parser.hpp"

#include "server/api/route_parameters_grammar.hpp"
#include "server/api/table_parameter_grammar.hpp"

namespace osrm
{
namespace server
{
namespace api
{

template<>
boost::optional<engine::api::RouteParameters> parseParameters(std::string::iterator& iter, std::string::iterator end)
{
    RouteParametersGrammar grammar;
    const auto result = boost::spirit::qi::parse(iter, end, grammar);

    boost::optional<engine::api::RouteParameters> parameters;
    if (result && iter == end)
    {
        parameters = std::move(grammar.parameters);
    }

    return parameters;
}

template<>
boost::optional<engine::api::TableParameters> parseParameters(std::string::iterator& iter, std::string::iterator end)
{
    TableParametersGrammar grammar;
    const auto result = boost::spirit::qi::parse(iter, end, grammar);

    boost::optional<engine::api::TableParameters> parameters;
    if (result && iter == end)
    {
        parameters = std::move(grammar.parameters);
    }

    return parameters;
}

}
}
}

