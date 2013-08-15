/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

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

#ifndef STRINGUTIL_H_
#define STRINGUTIL_H_

#include "../typedefs.h"

#include <boost/algorithm/string.hpp>

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>

#include <cstdio>
#include <string>

// precision:  position after decimal point
// length: maximum number of digits including comma and decimals
template< int length, int precision >
static inline char* printInt( char* buffer, int value ) {
    bool minus = false;
    if ( value < 0 ) {
        minus = true;
        value = -value;
    }
    buffer += length - 1;
    for ( int i = 0; i < precision; i++ ) {
        *buffer = '0' + ( value % 10 );
        value /= 10;
        buffer--;
    }
    *buffer = '.';
    buffer--;
    for ( int i = precision + 1; i < length; i++ ) {
        *buffer = '0' + ( value % 10 );
        value /= 10;
        if ( value == 0 ) break;
        buffer--;
    }
    if ( minus ) {
        buffer--;
        *buffer = '-';
    }
    return buffer;
}

static inline void intToString(const int value, std::string & output) {
    output.clear();
    std::back_insert_iterator<std::string> sink(output);
    boost::spirit::karma::generate(sink, boost::spirit::karma::int_, value);
}

static inline void int64ToString(const int64_t value, std::string & output) {
    output.clear();
    std::back_insert_iterator<std::string> sink(output);
    boost::spirit::karma::generate(sink, boost::spirit::karma::long_long, value);
}

static inline int stringToInt(const std::string& input) {
    std::string::const_iterator first_digit = input.begin();
    //Delete any trailing white-spaces
    while(first_digit != input.end() && std::isspace(*first_digit)) {
        ++first_digit;
    }
    int value = 0;
    boost::spirit::qi::parse(
        first_digit,
        input.end(),
        boost::spirit::int_, value
    );
    return value;
}

static inline unsigned stringToUint(const std::string& input) {
    std::string::const_iterator first_digit = input.begin();
    //Delete any trailing white-spaces
    while(first_digit != input.end() && std::isspace(*first_digit)) {
        ++first_digit;
    }
    int value = 0;
    boost::spirit::qi::parse(
        first_digit,
        input.end(),
        boost::spirit::uint_, value
    );
    return value;
}

static inline uint64_t stringToInt64(const std::string& input) {
    std::string::const_iterator first_digit = input.begin();
    //Delete any trailing white-spaces
    while(first_digit != input.end() && std::isspace(*first_digit)) {
        ++first_digit;
    }
    uint64_t value = 0;
    boost::spirit::qi::parse(
        first_digit,
        input.end(),
        boost::spirit::long_long, value
    );
    return value;
}


static inline void doubleToString(const double value, std::string & output){
    output.clear();
    std::back_insert_iterator<std::string> sink(output);
    boost::spirit::karma::generate(sink, boost::spirit::karma::double_, value);
}

static inline void doubleToStringWithTwoDigitsBehindComma(const double value, std::string & output){
    // The largest 32-bit integer is 4294967295, that is 10 chars
    // On the safe side, add 1 for sign, and 1 for trailing zero
    char buffer[12] ;
    sprintf(buffer, "%g", value) ;
    output = buffer ;
}

inline void replaceAll(std::string &s, const std::string &sub, const std::string &other) {
	boost::replace_all(s, sub, other);
}

inline void stringSplit(const std::string &s, const char delim, std::vector<std::string>& result) {
	boost::split(result, s, boost::is_any_of(std::string(&delim)));
}

static std::string originals[] = {"&", "\"",  "<",  ">", "'", "[", "]", "\\"};
static std::string entities[] = {"&amp;", "&quot;", "&lt;", "&gt;", "&#39;", "&91;", "&93;", " &#92;" };

inline std::string HTMLEntitize( const std::string & input) {
    std::string result(input);
    for(unsigned i = 0; i < sizeof(originals)/sizeof(std::string); ++i) {
        replaceAll(result, originals[i], entities[i]);
    }
    return result;
}

inline std::string HTMLDeEntitize( std::string & result) {
    for(unsigned i = 0; i < sizeof(originals)/sizeof(std::string); ++i) {
        replaceAll(result, entities[i], originals[i]);
    }
    return result;
}

inline bool StringStartsWith(const std::string & input, const std::string & prefix) {
    return boost::starts_with(input, prefix);
}

#endif /* STRINGUTIL_H_ */
