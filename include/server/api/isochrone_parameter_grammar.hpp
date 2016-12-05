#ifndef SERVER_API_ISOCHRONE_PARAMETERS_GRAMMAR_HPP
#define SERVER_API_ISOCHRONE_PARAMETERS_GRAMMAR_HPP

#include "engine/api/isochrone_parameters.hpp"

#include "engine/hint.hpp"
#include "engine/polyline_compressor.hpp"

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

#include <string>

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
          typename Signature = void(engine::api::IsochroneParameters &)>
struct IsochroneParametersGrammar final : boost::spirit::qi::grammar<Iterator, Signature>
{
    IsochroneParametersGrammar() : IsochroneParametersGrammar::base_type(root_rule)
    {
        root_rule =
            qi::lit("isochrone(") >
            qi::double_[ph::bind(&engine::api::IsochroneParameters::lon, qi::_r1) = qi::_1] > ',' >
            qi::double_[ph::bind(&engine::api::IsochroneParameters::lat, qi::_r1) = qi::_1] > ',' >
            qi::uint_[ph::bind(&engine::api::IsochroneParameters::range, qi::_r1) = qi::_1] >
            qi::lit(")");
    }

  private:
    qi::rule<Iterator, Signature> root_rule;
};
}
}
}

#endif
