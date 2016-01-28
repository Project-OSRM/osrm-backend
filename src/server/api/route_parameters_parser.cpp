#include "server/api/route_parameters_parser.hpp"
#include "server/api/route_parameters_grammar.hpp"

namespace osrm
{
namespace server
{
namespace api
{

boost::optional<engine::api::RouteParameters> parseRouteParameters(std::string::iterator& iter, std::string::iterator end)
{
    RouteParametersGrammar grammar;
    const auto result = boost::spirit::qi::parse(iter, end, grammar);

    boost::optional<engine::api::RouteParameters> parameters;
    if (result && iter == end)
    {
        parameters = std::move(grammar.route_parameters);
    }

    return parameters;
}

}
}
}

