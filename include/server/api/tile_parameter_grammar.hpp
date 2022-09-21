#ifndef SERVER_API_TILE_PARAMETERS_GRAMMAR_HPP
#define SERVER_API_TILE_PARAMETERS_GRAMMAR_HPP

#include "engine/api/tile_parameters.hpp"

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
} // namespace

template <typename Iterator = std::string::iterator,
          typename Signature = void(engine::api::TileParameters &)>
struct TileParametersGrammar final : boost::spirit::qi::grammar<Iterator, Signature>
{
    TileParametersGrammar() : TileParametersGrammar::base_type(root_rule)
    {
        root_rule = qi::lit("tile(") >
                    qi::uint_[ph::bind(&engine::api::TileParameters::x, qi::_r1) = qi::_1] > ',' >
                    qi::uint_[ph::bind(&engine::api::TileParameters::y, qi::_r1) = qi::_1] > ',' >
                    qi::uint_[ph::bind(&engine::api::TileParameters::z, qi::_r1) = qi::_1] >
                    qi::lit(").mvt");
    }

  private:
    qi::rule<Iterator, Signature> root_rule;
};
} // namespace api
} // namespace server
} // namespace osrm

#endif
