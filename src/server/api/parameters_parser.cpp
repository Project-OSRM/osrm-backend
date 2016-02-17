#include "server/api/parameters_parser.hpp"

#include "server/api/route_parameters_grammar.hpp"
#include "server/api/table_parameter_grammar.hpp"

#include <type_traits>

namespace osrm
{
namespace server
{
namespace api
{

namespace detail
{

template <typename ParameterT, typename GrammarT>
boost::optional<ParameterT> parseParameters(std::string::iterator &iter, std::string::iterator end)
{
    GrammarT grammar;
    const auto result = boost::spirit::qi::parse(iter, end, grammar);

    boost::optional<ParameterT> parameters;
    if (result && iter == end)
        parameters = std::move(grammar.parameters);

    return parameters;
}
} // ns detail

template <>
boost::optional<engine::api::RouteParameters> parseParameters(std::string::iterator &iter,
                                                              std::string::iterator end)
{
    return detail::parseParameters<engine::api::RouteParameters, RouteParametersGrammar>(iter, end);
}

template <>
boost::optional<engine::api::TableParameters> parseParameters(std::string::iterator &iter,
                                                              std::string::iterator end)
{
    return detail::parseParameters<engine::api::TableParameters, TableParametersGrammar>(iter, end);
}

} // ns api
} // ns server
} // ns osrm
