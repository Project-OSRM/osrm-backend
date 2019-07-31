#ifndef TABLE_PARAMETERS_GRAMMAR_HPP
#define TABLE_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"
#include "engine/api/table_parameters.hpp"

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

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
          typename Signature = void(engine::api::TableParameters &)>
struct TableParametersGrammar : public BaseParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = BaseParametersGrammar<Iterator, Signature>;

    TableParametersGrammar() : TableParametersGrammar(root_rule)
    {
#ifdef BOOST_HAS_LONG_LONG
        if (std::is_same<std::size_t, unsigned long long>::value)
            size_t_ = qi::ulong_long;
        else
            size_t_ = qi::ulong_;
#else
        size_t_ = qi::ulong_;
#endif

        destinations_rule =
            qi::lit("destinations=") >
            (qi::lit("all") |
             (size_t_ %
              ';')[ph::bind(&engine::api::TableParameters::destinations, qi::_r1) = qi::_1]);

        sources_rule =
            qi::lit("sources=") >
            (qi::lit("all") |
             (size_t_ % ';')[ph::bind(&engine::api::TableParameters::sources, qi::_r1) = qi::_1]);

        fallback_speed_rule =
            qi::lit("fallback_speed=") >
            (double_)[ph::bind(&engine::api::TableParameters::fallback_speed, qi::_r1) = qi::_1];

        fallback_coordinate_type.add("input",
                                     engine::api::TableParameters::FallbackCoordinateType::Input)(
            "snapped", engine::api::TableParameters::FallbackCoordinateType::Snapped);

        scale_factor_rule =
            qi::lit("scale_factor=") >
            (double_)[ph::bind(&engine::api::TableParameters::scale_factor, qi::_r1) = qi::_1];

        table_rule = destinations_rule(qi::_r1) | sources_rule(qi::_r1);

        root_rule = BaseGrammar::query_rule(qi::_r1) > BaseGrammar::format_rule(qi::_r1) >
                    -('?' > (table_rule(qi::_r1) | base_rule(qi::_r1) | scale_factor_rule(qi::_r1) |
                             fallback_speed_rule(qi::_r1) |
                             (qi::lit("fallback_coordinate=") >
                              fallback_coordinate_type
                                  [ph::bind(&engine::api::TableParameters::fallback_coordinate_type,
                                            qi::_r1) = qi::_1])) %
                                '&');
    }

    TableParametersGrammar(qi::rule<Iterator, Signature> &root_rule_) : BaseGrammar(root_rule_)
    {
        using AnnotationsType = engine::api::TableParameters::AnnotationsType;

        annotations.add("duration", AnnotationsType::Duration)("distance",
                                                               AnnotationsType::Distance);

        annotations_list = annotations[qi::_val |= qi::_1] % ',';

        base_rule = BaseGrammar::base_rule(qi::_r1) |
                    (qi::lit("annotations=") >
                     annotations_list[ph::bind(&engine::api::TableParameters::annotations,
                                               qi::_r1) = qi::_1]);
    }

  protected:
    qi::rule<Iterator, Signature> base_rule;

  private:
    using json_policy = no_trailing_dot_policy<double, 'j', 's', 'o', 'n'>;

    qi::rule<Iterator, Signature> root_rule;
    qi::rule<Iterator, Signature> table_rule;
    qi::rule<Iterator, Signature> sources_rule;
    qi::rule<Iterator, Signature> destinations_rule;
    qi::rule<Iterator, Signature> fallback_speed_rule;
    qi::rule<Iterator, Signature> scale_factor_rule;
    qi::rule<Iterator, std::size_t()> size_t_;
    qi::symbols<char, engine::api::TableParameters::AnnotationsType> annotations;
    qi::rule<Iterator, engine::api::TableParameters::AnnotationsType()> annotations_list;
    qi::symbols<char, engine::api::TableParameters::FallbackCoordinateType>
        fallback_coordinate_type;
    qi::real_parser<double, json_policy> double_;
};
}
}
}

#endif
