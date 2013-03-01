/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */



#ifndef APIGRAMMAR_H_
#define APIGRAMMAR_H_

#include <boost/bind.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_action.hpp>

namespace qi = boost::spirit::qi;

template <typename Iterator, class HandlerT>
struct APIGrammar : qi::grammar<Iterator> {
    APIGrammar(HandlerT * h) : APIGrammar::base_type(api_call), handler(h) {
        api_call = qi::lit('/') >> string[boost::bind(&HandlerT::setService, handler, ::_1)] >> *(query);
        query    = ('?') >> (+(zoom | output | jsonp | checksum | location | hint | cmp | language | instruction | geometry | alt_route | old_API) ) ;

        zoom        = (-qi::lit('&')) >> qi::lit('z')            >> '=' >> qi::short_[boost::bind(&HandlerT::setZoomLevel, handler, ::_1)];
        output      = (-qi::lit('&')) >> qi::lit("output")       >> '=' >> string[boost::bind(&HandlerT::setOutputFormat, handler, ::_1)];
        jsonp       = (-qi::lit('&')) >> qi::lit("jsonp")        >> '=' >> stringwithDot[boost::bind(&HandlerT::setJSONpParameter, handler, ::_1)];
        checksum    = (-qi::lit('&')) >> qi::lit("checksum")     >> '=' >> qi::int_[boost::bind(&HandlerT::setChecksum, handler, ::_1)];
        instruction = (-qi::lit('&')) >> qi::lit("instructions") >> '=' >> qi::bool_[boost::bind(&HandlerT::setInstructionFlag, handler, ::_1)];
        geometry    = (-qi::lit('&')) >> qi::lit("geometry")     >> '=' >> qi::bool_[boost::bind(&HandlerT::setGeometryFlag, handler, ::_1)];
        cmp         = (-qi::lit('&')) >> qi::lit("compression")  >> '=' >> qi::bool_[boost::bind(&HandlerT::setCompressionFlag, handler, ::_1)];
        location    = (-qi::lit('&')) >> qi::lit("loc")          >> '=' >> (qi::double_ >> qi::lit(',') >> qi::double_)[boost::bind(&HandlerT::addCoordinate, handler, ::_1)];
        hint        = (-qi::lit('&')) >> qi::lit("hint")         >> '=' >> stringwithDot[boost::bind(&HandlerT::addHint, handler, ::_1)];
        language    = (-qi::lit('&')) >> qi::lit("hl")           >> '=' >> string[boost::bind(&HandlerT::setLanguage, handler, ::_1)];
        alt_route   = (-qi::lit('&')) >> qi::lit("alt")          >> '=' >> qi::bool_[boost::bind(&HandlerT::setAlternateRouteFlag, handler, ::_1)];
        old_API     = (-qi::lit('&')) >> qi::lit("geomformat")   >> '=' >> string[boost::bind(&HandlerT::setDeprecatedAPIFlag, handler, ::_1)];

        string        = +(qi::char_("a-zA-Z"));
        stringwithDot = +(qi::char_("a-zA-Z0-9_.-"));
    }
    qi::rule<Iterator> api_call, query;
    qi::rule<Iterator, std::string()> service, zoom, output, string, jsonp, checksum, location, hint,
                                      stringwithDot, language, instruction, geometry,
                                      cmp, alt_route, old_API;

    HandlerT * handler;
};


#endif /* APIGRAMMAR_H_ */
