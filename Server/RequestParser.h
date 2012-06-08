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

#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>
#include "BasicDatastructures.h"

namespace http {

class RequestParser {
public:
    RequestParser() : state_(method_start) { }
    void Reset() { state_ = method_start; }

    boost::tuple<boost::tribool, char*> Parse(Request& req, char* begin, char* end, CompressionType * compressionType) {
        while (begin != end) {
            boost::tribool result = consume(req, *begin++, compressionType);
            if (result || !result){
                return boost::make_tuple(result, begin);
            }
        }
        boost::tribool result = boost::indeterminate;
        return boost::make_tuple(result, begin);
    }

private:
    boost::tribool consume(Request& req, char input, CompressionType * compressionType) {
        switch (state_) {
        case method_start:
            if (!isChar(input) || isCTL(input) || isTSpecial(input)) {
                return false;
            } else {
                state_ = method;
                return boost::indeterminate;
            }
        case method:
            if (input == ' ') {
                state_ = uri;
                return boost::indeterminate;
            } else if (!isChar(input) || isCTL(input) || isTSpecial(input)) {
                return false;
            } else {
                return boost::indeterminate;
            }
        case uri_start:
            if (isCTL(input)) {
                return false;
            } else {
                state_ = uri;
                req.uri.push_back(input);
                return boost::indeterminate;
            }
        case uri:
            if (input == ' ') {
                state_ = http_version_h;
                return boost::indeterminate;
            } else if (isCTL(input)) {
                return false;
            } else {
                req.uri.push_back(input);
                return boost::indeterminate;
            }
        case http_version_h:
            if (input == 'H') {
                state_ = http_version_t_1;
                return boost::indeterminate;
            } else {
                return false;
            }
        case http_version_t_1:
            if (input == 'T') {
                state_ = http_version_t_2;
                return boost::indeterminate;
            } else {
                return false;
            }
        case http_version_t_2:
            if (input == 'T') {
                state_ = http_version_p;
                return boost::indeterminate;
            } else {
                return false;
            }
        case http_version_p:
            if (input == 'P') {
                state_ = http_version_slash;
                return boost::indeterminate;
            } else {
                return false;
            }
        case http_version_slash:
            if (input == '/') {
                state_ = http_version_major_start;
                return boost::indeterminate;
            } else {
                return false;
            }
        case http_version_major_start:
            if (isDigit(input)) {
                state_ = http_version_major;
                return boost::indeterminate;
            } else {
                return false;
            }
        case http_version_major:
            if (input == '.') {
                state_ = http_version_minor_start;
                return boost::indeterminate;
            } else if (isDigit(input)) {
                return boost::indeterminate;
            } else {
                return false;
            }
        case http_version_minor_start:
            if (isDigit(input)) {
                state_ = http_version_minor;
                return boost::indeterminate;
            } else {
                return false;
            }
        case http_version_minor:
            if (input == '\r') {
                state_ = expecting_newline_1;
                return boost::indeterminate;
            } else if (isDigit(input)) {
                return boost::indeterminate;
            }
            else {
                return false;
            }
        case expecting_newline_1:
            if (input == '\n') {
                state_ = header_line_start;
                return boost::indeterminate;
            } else {
                return false;
            }
        case header_line_start:
            if(header.name == "Accept-Encoding") {
                /* giving gzip precedence over deflate */
                if(header.value.find("deflate") != std::string::npos)
                    *compressionType = deflateRFC1951;
                if(header.value.find("gzip") != std::string::npos)
                    *compressionType = gzipRFC1952;
            }

            if("Referer" == header.name)
                req.referrer = header.value;

            if("User-Agent" == header.name)
                req.agent = header.value;

            if (input == '\r') {
                state_ = expecting_newline_3;
                return boost::indeterminate;
            } else if (!isChar(input) || isCTL(input) || isTSpecial(input)) {
                return false;
            } else {
                state_ = header_name;
                header.Clear();
                header.name.push_back(input);
                return boost::indeterminate;
            }
        case header_lws:
            if (input == '\r') {
                state_ = expecting_newline_2;
                return boost::indeterminate;
            } else if (input == ' ' || input == '\t') {
                return boost::indeterminate;
            }
            else if (isCTL(input)) {
                return false;
            } else {
                state_ = header_value;
                return boost::indeterminate;
            }
        case header_name:
            if (input == ':') {
                state_ = space_before_header_value;
                return boost::indeterminate;
            } else if (!isChar(input) || isCTL(input) || isTSpecial(input)) {
                return false;
            } else {
                header.name.push_back(input);
                return boost::indeterminate;
            }
        case space_before_header_value:
            if (input == ' ') {
                state_ = header_value;
                return boost::indeterminate;
            } else {
                return false;
            }
        case header_value:
            if (input == '\r') {
                state_ = expecting_newline_2;
                return boost::indeterminate;
            } else if (isCTL(input)) {
                return false;
            } else {
                header.value.push_back(input);
                return boost::indeterminate;
            }
        case expecting_newline_2:
            if (input == '\n') {
                state_ = header_line_start;
                return boost::indeterminate;
            } else {
                return false;
            }
        case expecting_newline_3:
            return (input == '\n');
        default:
            return false;
        }
    }

    inline bool isChar(int c) {
        return c >= 0 && c <= 127;
    }

    inline bool isCTL(int c) {
        return (c >= 0 && c <= 31) || (c == 127);
    }

    inline bool isTSpecial(int c) {
        switch (c) {
        case '(': case ')': case '<': case '>': case '@':
        case ',': case ';': case ':': case '\\': case '"':
        case '/': case '[': case ']': case '?': case '=':
        case '{': case '}': case ' ': case '\t':
            return true;
        default:
            return false;
        }
    }

    inline bool isDigit(int c) {
        return c >= '0' && c <= '9';
    }

    enum state {
        method_start,
        method,
        uri_start,
        uri,
        http_version_h,
        http_version_t_1,
        http_version_t_2,
        http_version_p,
        http_version_slash,
        http_version_major_start,
        http_version_major,
        http_version_minor_start,
        http_version_minor,
        expecting_newline_1,
        header_line_start,
        header_lws,
        header_name,
        space_before_header_value,
        header_value,
        expecting_newline_2,
        expecting_newline_3
    } state_;

    Header header;
};

} // namespace http

#endif // REQUEST_PARSER_H
