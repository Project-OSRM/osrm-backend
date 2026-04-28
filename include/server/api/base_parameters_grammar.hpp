#ifndef SERVER_API_BASE_PARAMETERS_GRAMMAR_HPP
#define SERVER_API_BASE_PARAMETERS_GRAMMAR_HPP

#include "engine/api/base_parameters.hpp"

#include "engine/bearing.hpp"
#include "engine/hint.hpp"
#include "engine/polyline_compressor.hpp"

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/spirit/home/x3.hpp>

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <vector>

BOOST_FUSION_ADAPT_STRUCT(osrm::engine::Bearing, bearing, range)

namespace osrm::server::api
{

namespace x3 = boost::spirit::x3;

// Context tag for passing parameters struct through X3 parsing context
struct params_tag;

// Custom real parser policy: prevents interpreting ".json"/".flatbuffers" as decimal point
template <typename T, char... Fmt> struct no_trailing_dot_policy : x3::real_policies<T>
{
    template <typename Iterator> static bool parse_dot(Iterator &first, Iterator const &last)
    {
        auto diff = std::distance(first, last);
        if (diff <= 0 || *first != '.')
            return false;

        static const constexpr char fmt[sizeof...(Fmt)] = {Fmt...};

        if (sizeof(fmt) < static_cast<size_t>(diff) &&
            std::equal(fmt, fmt + sizeof(fmt), first + 1u))
            return false;

        ++first;
        return true;
    }

    template <typename Iterator> static bool parse_exp(Iterator &, const Iterator &)
    {
        return false;
    }

    template <typename Iterator, typename Attribute>
    static bool parse_exp_n(Iterator &, const Iterator &, Attribute &)
    {
        return false;
    }

    template <typename Iterator, typename Attribute>
    static bool parse_nan(Iterator &, const Iterator &, Attribute &)
    {
        return false;
    }

    template <typename Iterator, typename Attribute>
    static bool parse_inf(Iterator &, const Iterator &, Attribute &)
    {
        return false;
    }
};

namespace base_grammar
{

using json_policy = no_trailing_dot_policy<double, 'j', 's', 'o', 'n'>;
inline const x3::real_parser<double, json_policy> json_double{};

// --- Primitive character parsers ---
inline const auto polyline_chars = x3::char_("a-zA-Z0-9_.--[]{}@?|\\%~`^");
inline const auto base64_char = x3::char_("a-zA-Z0-9--_=");

// --- Value parsers (synthesize attributes) ---

inline const auto unlimited = x3::rule<struct unlimited_tag, double>{
    "unlimited"} = x3::lit("unlimited") >> x3::attr(std::numeric_limits<double>::infinity());

// "bearing,range" -> Bearing (via Fusion adaptation)
inline const auto bearing_rule = x3::rule<struct bearing_tag, engine::Bearing>{"bearing"} =
    x3::short_ > ',' > x3::short_;

// "lon,lat" -> Coordinate
inline const auto location_rule = x3::rule<struct location_tag, util::Coordinate>{"location"} =
    (json_double > ',' > json_double)[(
        [](auto &ctx)
        {
            auto &attr = x3::_attr(ctx);
            try
            {
                x3::_val(ctx) = util::Coordinate(
                    util::toFixed(util::UnsafeFloatLongitude{boost::fusion::at_c<0>(attr)}),
                    util::toFixed(util::UnsafeFloatLatitude{boost::fusion::at_c<1>(attr)}));
            }
            catch (const boost::numeric::bad_numeric_cast &)
            {
                x3::_pass(ctx) = false;
            }
        })];

inline const auto polyline_content =
    x3::rule<struct polyline_content_tag, std::string>{"polyline_content"} = +polyline_chars;

inline const auto polyline_rule =
    x3::rule<struct polyline_tag, std::vector<util::Coordinate>>{"polyline"} =
        (x3::lit("polyline(") > polyline_content >
         ')')[([](auto &ctx) { x3::_val(ctx) = engine::decodePolyline(x3::_attr(ctx)); })];

inline const auto polyline6_rule =
    x3::rule<struct polyline6_tag, std::vector<util::Coordinate>>{"polyline6"} =
        (x3::lit("polyline6(") > polyline_content >
         ')')[([](auto &ctx) { x3::_val(ctx) = engine::decodePolyline<1000000>(x3::_attr(ctx)); })];

inline const auto hint_string = x3::rule<struct hint_string_tag, std::string>{"hint_string"} =
    x3::repeat(engine::ENCODED_SEGMENT_HINT_SIZE)[base64_char];

// --- Symbol tables ---

inline const auto approach_type = []()
{
    x3::symbols<engine::Approach> sym;
    sym.add("unrestricted", engine::Approach::UNRESTRICTED)("curb", engine::Approach::CURB)(
        "opposite", engine::Approach::OPPOSITE);
    return sym;
}();

inline const auto snapping_type = []()
{
    x3::symbols<engine::api::BaseParameters::SnappingType> sym;
    sym.add("default", engine::api::BaseParameters::SnappingType::Default)(
        "any", engine::api::BaseParameters::SnappingType::Any);
    return sym;
}();

inline const auto format_type = []()
{
    x3::symbols<engine::api::BaseParameters::OutputFormatType> sym;
    sym.add(".json", engine::api::BaseParameters::OutputFormatType::JSON)(
        ".flatbuffers", engine::api::BaseParameters::OutputFormatType::FLATBUFFERS);
    return sym;
}();

// --- Intermediate typed rules to collapse variant alternatives ---

// Wrap alternatives with same attribute type in a typed rule so X3 doesn't produce variant
inline const auto coords_parser =
    x3::rule<struct coords_parser_tag, std::vector<util::Coordinate>>{"coords"} =
        (location_rule % ';') | polyline_rule | polyline6_rule;

inline const auto radius_value = x3::rule<struct radius_value_tag, double>{"radius_value"} =
    json_double | unlimited;

// --- Parameter rules (modify params via context, unused attribute) ---

inline const auto query_rule = x3::rule<struct base_query_tag>{"base_query"} =
    coords_parser[([](auto &ctx) { x3::get<params_tag>(ctx).get().coordinates = x3::_attr(ctx); })];

inline const auto format_rule = x3::rule<struct base_format_tag>{"base_format"} =
    -format_type[([](auto &ctx) { x3::get<params_tag>(ctx).get().format = x3::_attr(ctx); })];

inline const auto radiuses_rule = x3::lit("radiuses=") >
                                  (-radius_value)[(
                                      [](auto &ctx)
                                      {
                                          auto &params = x3::get<params_tag>(ctx).get();
                                          auto &opt = x3::_attr(ctx);
                                          params.radiuses.push_back(opt ? std::make_optional(*opt)
                                                                        : std::nullopt);
                                      })] %
                                      ';';

inline const auto
    hints_rule = x3::lit("hints=") >
                 (*hint_string)[(
                     [](auto &ctx)
                     {
                         auto &params = x3::get<params_tag>(ctx).get();
                         auto &hint_strings = x3::_attr(ctx);
                         if (!hint_strings.empty())
                         {
                             std::vector<engine::SegmentHint> location_hints(hint_strings.size());
                             std::transform(hint_strings.begin(),
                                            hint_strings.end(),
                                            location_hints.begin(),
                                            [](const auto &hs)
                                            { return engine::SegmentHint::FromBase64(hs); });
                             params.hints.push_back(engine::Hint{std::move(location_hints)});
                         }
                         else
                         {
                             params.hints.emplace_back(std::nullopt);
                         }
                     })] %
                     ';';

inline const auto bearings_rule = x3::lit("bearings=") >
                                  (-(x3::short_ > ',' > x3::short_))[(
                                      [](auto &ctx)
                                      {
                                          auto &params = x3::get<params_tag>(ctx).get();
                                          auto &opt = x3::_attr(ctx);
                                          if (opt)
                                          {
                                              params.bearings.push_back(
                                                  engine::Bearing{boost::fusion::at_c<0>(*opt),
                                                                  boost::fusion::at_c<1>(*opt)});
                                          }
                                          else
                                          {
                                              params.bearings.push_back(std::nullopt);
                                          }
                                      })] %
                                      ';';

inline const auto generate_hints_rule =
    x3::lit("generate_hints=") >
    x3::bool_[([](auto &ctx) { x3::get<params_tag>(ctx).get().generate_hints = x3::_attr(ctx); })];

inline const auto skip_waypoints_rule =
    x3::lit("skip_waypoints=") >
    x3::bool_[([](auto &ctx) { x3::get<params_tag>(ctx).get().skip_waypoints = x3::_attr(ctx); })];

inline const auto approach_rule = x3::lit("approaches=") >
                                  (-approach_type)[(
                                      [](auto &ctx)
                                      {
                                          auto &params = x3::get<params_tag>(ctx).get();
                                          auto &opt = x3::_attr(ctx);
                                          params.approaches.push_back(opt ? std::make_optional(*opt)
                                                                          : std::nullopt);
                                      })] %
                                      ';';

inline const auto snapping_rule =
    x3::lit("snapping=") >
    snapping_type[([](auto &ctx) { x3::get<params_tag>(ctx).get().snapping = x3::_attr(ctx); })];

inline const auto exclude_item = x3::rule<struct exclude_item_tag, std::string>{"exclude_item"} =
    +x3::char_("a-zA-Z0-9");

inline const auto exclude_rule =
    x3::lit("exclude=") >
    (exclude_item %
     ',')[([](auto &ctx) { x3::get<params_tag>(ctx).get().exclude = x3::_attr(ctx); })];

// Combined base options
inline const auto base_options = x3::rule<struct base_options_tag>{"base_options"} =
    radiuses_rule | hints_rule | bearings_rule | generate_hints_rule | skip_waypoints_rule |
    approach_rule | exclude_rule | snapping_rule;

} // namespace base_grammar
} // namespace osrm::server::api

#endif
