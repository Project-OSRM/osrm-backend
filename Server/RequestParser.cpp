/*

Copyright (c) 2015, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "RequestParser.h"

#include "Http/Request.h"

namespace http
{

RequestParser::RequestParser()
    : state_(method_start), header({"", ""}), compression_type(noCompression)
{
}

void RequestParser::Reset() { state_ = method_start; }

std::tuple<osrm::tribool, CompressionType>
RequestParser::Parse(Request &req, char *begin, char *end)
{
    while (begin != end)
    {
        osrm::tribool result = consume(req, *begin++);
        if (result == osrm::tribool::yes || result == osrm::tribool::no)
        {
            return std::make_tuple(result, compression_type);
        }
    }
    osrm::tribool result = osrm::tribool::indeterminate;
    return std::make_tuple(result, compression_type);
}

osrm::tribool RequestParser::consume(Request &req, char input)
{
    switch (state_)
    {
    case method_start:
        if (!isChar(input) || isCTL(input) || isTSpecial(input))
        {
            return osrm::tribool::no;
        }
        state_ = method;
        return osrm::tribool::indeterminate;
    case method:
        if (input == ' ')
        {
            state_ = uri;
            return osrm::tribool::indeterminate;
        }
        if (!isChar(input) || isCTL(input) || isTSpecial(input))
        {
            return osrm::tribool::no;
        }
        return osrm::tribool::indeterminate;
    case uri_start:
        if (isCTL(input))
        {
            return osrm::tribool::no;
        }
        state_ = uri;
        req.uri.push_back(input);
        return osrm::tribool::indeterminate;
    case uri:
        if (input == ' ')
        {
            state_ = http_version_h;
            return osrm::tribool::indeterminate;
        }
        if (isCTL(input))
        {
            return osrm::tribool::no;
        }
        req.uri.push_back(input);
        return osrm::tribool::indeterminate;
    case http_version_h:
        if (input == 'H')
        {
            state_ = http_version_t_1;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case http_version_t_1:
        if (input == 'T')
        {
            state_ = http_version_t_2;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case http_version_t_2:
        if (input == 'T')
        {
            state_ = http_version_p;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case http_version_p:
        if (input == 'P')
        {
            state_ = http_version_slash;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case http_version_slash:
        if (input == '/')
        {
            state_ = http_version_major_start;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case http_version_major_start:
        if (isDigit(input))
        {
            state_ = http_version_major;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case http_version_major:
        if (input == '.')
        {
            state_ = http_version_minor_start;
            return osrm::tribool::indeterminate;
        }
        if (isDigit(input))
        {
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case http_version_minor_start:
        if (isDigit(input))
        {
            state_ = http_version_minor;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case http_version_minor:
        if (input == '\r')
        {
            state_ = expecting_newline_1;
            return osrm::tribool::indeterminate;
        }
        if (isDigit(input))
        {
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case expecting_newline_1:
        if (input == '\n')
        {
            state_ = header_line_start;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case header_line_start:
        if (header.name == "Accept-Encoding")
        {
            /* giving gzip precedence over deflate */
            if (header.value.find("deflate") != std::string::npos)
            {
                compression_type = deflateRFC1951;
            }
            if (header.value.find("gzip") != std::string::npos)
            {
                compression_type = gzipRFC1952;
            }
        }

        if ("Referer" == header.name)
        {
            req.referrer = header.value;
        }

        if ("User-Agent" == header.name)
        {
            req.agent = header.value;
        }

        if (input == '\r')
        {
            state_ = expecting_newline_3;
            return osrm::tribool::indeterminate;
        }
        if (!isChar(input) || isCTL(input) || isTSpecial(input))
        {
            return osrm::tribool::no;
        }
        state_ = header_name;
        header.Clear();
        header.name.push_back(input);
        return osrm::tribool::indeterminate;
    case header_lws:
        if (input == '\r')
        {
            state_ = expecting_newline_2;
            return osrm::tribool::indeterminate;
        }
        if (input == ' ' || input == '\t')
        {
            return osrm::tribool::indeterminate;
        }
        if (isCTL(input))
        {
            return osrm::tribool::no;
        }
        state_ = header_value;
        return osrm::tribool::indeterminate;
    case header_name:
        if (input == ':')
        {
            state_ = space_before_header_value;
            return osrm::tribool::indeterminate;
        }
        if (!isChar(input) || isCTL(input) || isTSpecial(input))
        {
            return osrm::tribool::no;
        }
        header.name.push_back(input);
        return osrm::tribool::indeterminate;
    case space_before_header_value:
        if (input == ' ')
        {
            state_ = header_value;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case header_value:
        if (input == '\r')
        {
            state_ = expecting_newline_2;
            return osrm::tribool::indeterminate;
        }
        if (isCTL(input))
        {
            return osrm::tribool::no;
        }
        header.value.push_back(input);
        return osrm::tribool::indeterminate;
    case expecting_newline_2:
        if (input == '\n')
        {
            state_ = header_line_start;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    default: // expecting_newline_3
        return (input == '\n' ? osrm::tribool::yes : osrm::tribool::no);
        // default:
        //     return osrm::tribool::no;
    }
}

inline bool RequestParser::isChar(int character) { return character >= 0 && character <= 127; }

inline bool RequestParser::isCTL(int character)
{
    return (character >= 0 && character <= 31) || (character == 127);
}

inline bool RequestParser::isTSpecial(int character)
{
    switch (character)
    {
    case '(':
    case ')':
    case '<':
    case '>':
    case '@':
    case ',':
    case ';':
    case ':':
    case '\\':
    case '"':
    case '/':
    case '[':
    case ']':
    case '?':
    case '=':
    case '{':
    case '}':
    case ' ':
    case '\t':
        return true;
    default:
        return false;
    }
}

inline bool RequestParser::isDigit(int character) { return character >= '0' && character <= '9'; }
}
