#ifndef SERVER_API_TILE_PARAMETERS_GRAMMAR_HPP
#define SERVER_API_TILE_PARAMETERS_GRAMMAR_HPP

#include "engine/api/tile_parameters.hpp"

#include "engine/polyline_compressor.hpp"
#include "engine/hint.hpp"

#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/spirit/include/qi_grammar.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_operator.hpp>

#include <string>

namespace osrm
{
namespace server
{
namespace api
{

namespace qi = boost::spirit::qi;
struct TileParametersGrammar final : boost::spirit::qi::grammar<std::string::iterator>
{
    using Iterator = std::string::iterator;

    TileParametersGrammar()
        : TileParametersGrammar::base_type(root_rule)
    {
        const auto set_x = [this](const unsigned x_) { parameters.x = x_; };
        const auto set_y = [this](const unsigned y_) { parameters.y = y_; };
        const auto set_z = [this](const unsigned z_) { parameters.z = z_; };

        query_rule = qi::lit("tile(") >> qi::uint_[set_x] >> qi::lit(",") >> qi::uint_[set_y] >> qi::lit(",") >> qi::uint_[set_z] >> qi::lit(")");

        root_rule = query_rule >> qi::lit(".mvt");
    }
    engine::api::TileParameters parameters;

private:
    qi::rule<Iterator> root_rule;
    qi::rule<Iterator> query_rule;
};
}
}
}

#endif
