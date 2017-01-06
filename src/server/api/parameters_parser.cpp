#include "server/api/parameters_parser.hpp"

#include "server/api/match_parameter_grammar.hpp"
#include "server/api/nearest_parameter_grammar.hpp"
#include "server/api/route_parameters_grammar.hpp"
#include "server/api/table_parameter_grammar.hpp"
#include "server/api/tile_parameter_grammar.hpp"
#include "server/api/trip_parameter_grammar.hpp"

#include <type_traits>

namespace osrm
{
namespace server
{
namespace api
{

namespace detail
{
template <typename T>
using is_grammar_t =
    std::integral_constant<bool,
                           std::is_same<RouteParametersGrammar<>, T>::value ||
                               std::is_same<TableParametersGrammar<>, T>::value ||
                               std::is_same<NearestParametersGrammar<>, T>::value ||
                               std::is_same<TripParametersGrammar<>, T>::value ||
                               std::is_same<MatchParametersGrammar<>, T>::value ||
                               std::is_same<TileParametersGrammar<>, T>::value>;

template <typename ParameterT,
          typename GrammarT,
          typename std::enable_if<detail::is_parameter_t<ParameterT>::value, int>::type = 0,
          typename std::enable_if<detail::is_grammar_t<GrammarT>::value, int>::type = 0>
boost::optional<ParameterT> parseParameters(std::string::iterator &iter,
                                            const std::string::iterator end)
{
    using It = std::decay<decltype(iter)>::type;

    static const GrammarT grammar;

    try
    {
        ParameterT parameters;
        const auto ok =
            boost::spirit::qi::parse(iter, end, grammar(boost::phoenix::ref(parameters)));

        // return move(a.b) is needed to move b out of a and then return the rvalue by implicit move
        if (ok && iter == end)
            return std::move(parameters);
    }
    catch (const qi::expectation_failure<It> &failure)
    {
        // The grammar above using expectation parsers ">" does not automatically increment the
        // iterator to the failing position. Extract the position from the exception ourselves.
        iter = failure.first;
    }
    catch (const boost::numeric::bad_numeric_cast &)
    {
        // this can happen if we get bad numeric values in the request, just handle
        // as normal parser error
    }

    return boost::none;
}
} // ns detail

template <>
boost::optional<engine::api::RouteParameters> parseParameters(std::string::iterator &iter,
                                                              const std::string::iterator end)
{
    return detail::parseParameters<engine::api::RouteParameters, RouteParametersGrammar<>>(iter,
                                                                                           end);
}

template <>
boost::optional<engine::api::TableParameters> parseParameters(std::string::iterator &iter,
                                                              const std::string::iterator end)
{
    return detail::parseParameters<engine::api::TableParameters, TableParametersGrammar<>>(iter,
                                                                                           end);
}

template <>
boost::optional<engine::api::NearestParameters> parseParameters(std::string::iterator &iter,
                                                                const std::string::iterator end)
{
    return detail::parseParameters<engine::api::NearestParameters, NearestParametersGrammar<>>(iter,
                                                                                               end);
}

template <>
boost::optional<engine::api::TripParameters> parseParameters(std::string::iterator &iter,
                                                             const std::string::iterator end)
{
    return detail::parseParameters<engine::api::TripParameters, TripParametersGrammar<>>(iter, end);
}

template <>
boost::optional<engine::api::MatchParameters> parseParameters(std::string::iterator &iter,
                                                              const std::string::iterator end)
{
    return detail::parseParameters<engine::api::MatchParameters, MatchParametersGrammar<>>(iter,
                                                                                           end);
}

template <>
boost::optional<engine::api::TileParameters> parseParameters(std::string::iterator &iter,
                                                             const std::string::iterator end)
{
    return detail::parseParameters<engine::api::TileParameters, TileParametersGrammar<>>(iter, end);
}

} // ns api
} // ns server
} // ns osrm
