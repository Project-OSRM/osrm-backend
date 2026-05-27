#include "server/api/parameters_parser.hpp"

#include "server/api/match_parameter_grammar.hpp"
#include "server/api/nearest_parameter_grammar.hpp"
#include "server/api/route_parameters_grammar.hpp"
#include "server/api/table_parameter_grammar.hpp"
#include "server/api/tile_parameter_grammar.hpp"
#include "server/api/trip_parameter_grammar.hpp"

#include <type_traits>

namespace osrm::server::api
{

namespace detail
{

namespace x3 = boost::spirit::x3;

template <typename ParameterT, typename Grammar>
std::optional<ParameterT> parseParameters(std::string::iterator &iter,
                                          const std::string::iterator end,
                                          const Grammar &grammar)
{
    try
    {
        ParameterT parameters;
        const auto ok = x3::parse(iter, end, x3::with<params_tag>(std::ref(parameters))[grammar]);

        if (ok && iter == end)
            return parameters;
    }
    catch (const x3::expectation_failure<std::string::iterator> &failure)
    {
        // The grammar above using expectation parsers ">" does not automatically increment the
        // iterator to the failing position. Extract the position from the exception ourselves.
        iter = failure.where();
    }
    catch (const boost::numeric::bad_numeric_cast &)
    {
        // this can happen if we get bad numeric values in the request, just handle
        // as normal parser error
        return std::nullopt;
    }

    return std::nullopt;
}

} // namespace detail

template <>
std::optional<engine::api::RouteParameters> parseParameters(std::string::iterator &iter,
                                                            const std::string::iterator end)
{
    return detail::parseParameters<engine::api::RouteParameters>(
        iter, end, route_grammar::root_rule);
}

template <>
std::optional<engine::api::TableParameters> parseParameters(std::string::iterator &iter,
                                                            const std::string::iterator end)
{
    return detail::parseParameters<engine::api::TableParameters>(
        iter, end, table_grammar::root_rule);
}

template <>
std::optional<engine::api::NearestParameters> parseParameters(std::string::iterator &iter,
                                                              const std::string::iterator end)
{
    return detail::parseParameters<engine::api::NearestParameters>(
        iter, end, nearest_grammar::root_rule);
}

template <>
std::optional<engine::api::TripParameters> parseParameters(std::string::iterator &iter,
                                                           const std::string::iterator end)
{
    return detail::parseParameters<engine::api::TripParameters>(iter, end, trip_grammar::root_rule);
}

template <>
std::optional<engine::api::MatchParameters> parseParameters(std::string::iterator &iter,
                                                            const std::string::iterator end)
{
    return detail::parseParameters<engine::api::MatchParameters>(
        iter, end, match_grammar::root_rule);
}

template <>
std::optional<engine::api::TileParameters> parseParameters(std::string::iterator &iter,
                                                           const std::string::iterator end)
{
    namespace x3 = boost::spirit::x3;
    try
    {
        engine::api::TileParameters parameters;
        const auto ok = x3::parse(iter, end, tile_rule, parameters);

        if (ok && iter == end)
            return parameters;
    }
    catch (const x3::expectation_failure<std::string::iterator> &failure)
    {
        iter = failure.where();
    }

    return std::nullopt;
}

} // namespace osrm::server::api
