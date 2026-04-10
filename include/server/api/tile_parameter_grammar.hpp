#ifndef SERVER_API_TILE_PARAMETERS_GRAMMAR_HPP
#define SERVER_API_TILE_PARAMETERS_GRAMMAR_HPP

#include "engine/api/tile_parameters.hpp"

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>

BOOST_FUSION_ADAPT_STRUCT(osrm::engine::api::TileParameters, x, y, z)

namespace osrm::server::api
{

namespace x3 = boost::spirit::x3;

// X3 rule: parses "tile(x,y,z).mvt" into TileParameters
const x3::rule<struct tile_rule_tag, engine::api::TileParameters> tile_rule = "tile_rule";
const auto tile_rule_def = x3::lit("tile(") > x3::uint_ > ',' > x3::uint_ > ',' > x3::uint_ >
                           x3::lit(").mvt");
BOOST_SPIRIT_DEFINE(tile_rule)

} // namespace osrm::server::api

#endif
