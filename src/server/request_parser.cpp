#include "server/request_parser.hpp"

#include "server/http/compression_type.hpp"
#include "server/http/header.hpp"
#include "server/http/request.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <string>

namespace osrm
{
namespace server
{

RequestParser::RequestParser()
    : state(internal_state::method_start), current_header({"", ""}),
      selected_compression(http::no_compression)
{
}

std::tuple<RequestParser::RequestStatus, http::compression_type>
RequestParser::parse(http::request &current_request, char *begin, char *end)
{
    while (begin != end)
    {
        RequestStatus result = consume(current_request, *begin++);
        if (result != RequestStatus::indeterminate)
        {
            return std::make_tuple(result, selected_compression);
        }
    }
    RequestStatus result = RequestStatus::indeterminate;

    return std::make_tuple(result, selected_compression);
}

RequestParser::RequestStatus RequestParser::consume(http::request &current_request,
                                                    const char input)
{
    switch (state)
    {
    case internal_state::method_start:
        if (!is_char(input) || is_CTL(input) || is_special(input))
        {
            return RequestStatus::invalid;
        }
        state = internal_state::method;
        return RequestStatus::indeterminate;
    case internal_state::method:
        if (input == ' ')
        {
            state = internal_state::uri;
            return RequestStatus::indeterminate;
        }
        if (!is_char(input) || is_CTL(input) || is_special(input))
        {
            return RequestStatus::invalid;
        }
        return RequestStatus::indeterminate;
    case internal_state::uri_start:
        if (is_CTL(input))
        {
            return RequestStatus::invalid;
        }
        state = internal_state::uri;
        current_request.uri.push_back(input);
        return RequestStatus::indeterminate;
    case internal_state::uri:
        if (input == ' ')
        {
            state = internal_state::http_version_h;
            return RequestStatus::indeterminate;
        }
        if (is_CTL(input))
        {
            return RequestStatus::invalid;
        }
        current_request.uri.push_back(input);
        return RequestStatus::indeterminate;
    case internal_state::http_version_h:
        if (input == 'H')
        {
            state = internal_state::http_version_t_1;
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::http_version_t_1:
        if (input == 'T')
        {
            state = internal_state::http_version_t_2;
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::http_version_t_2:
        if (input == 'T')
        {
            state = internal_state::http_version_p;
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::http_version_p:
        if (input == 'P')
        {
            state = internal_state::http_version_slash;
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::http_version_slash:
        if (input == '/')
        {
            state = internal_state::http_version_major_start;
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::http_version_major_start:
        if (is_digit(input))
        {
            state = internal_state::http_version_major;
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::http_version_major:
        if (input == '.')
        {
            state = internal_state::http_version_minor_start;
            return RequestStatus::indeterminate;
        }
        if (is_digit(input))
        {
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::http_version_minor_start:
        if (is_digit(input))
        {
            state = internal_state::http_version_minor;
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::http_version_minor:
        if (input == '\r')
        {
            state = internal_state::expecting_newline_1;
            return RequestStatus::indeterminate;
        }
        if (is_digit(input))
        {
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::expecting_newline_1:
        if (input == '\n')
        {
            state = internal_state::header_line_start;
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::header_line_start:
        if (boost::iequals(current_header.name, "Accept-Encoding"))
        {
            /* giving gzip precedence over deflate */
            if (boost::icontains(current_header.value, "deflate"))
            {
                selected_compression = http::deflate_rfc1951;
            }
            if (boost::icontains(current_header.value, "gzip"))
            {
                selected_compression = http::gzip_rfc1952;
            }
        }

        if (boost::iequals(current_header.name, "Referer"))
        {
            current_request.referrer = current_header.value;
        }

        if (boost::iequals(current_header.name, "User-Agent"))
        {
            current_request.agent = current_header.value;
        }

        if (boost::iequals(current_header.name, "Connection"))
        {
            current_request.connection = current_header.value;
        }

        if (input == '\r')
        {
            state = internal_state::expecting_newline_3;
            return RequestStatus::indeterminate;
        }
        if (!is_char(input) || is_CTL(input) || is_special(input))
        {
            return RequestStatus::invalid;
        }
        state = internal_state::header_name;
        current_header.clear();
        current_header.name.push_back(input);
        return RequestStatus::indeterminate;
    case internal_state::header_lws:
        if (input == '\r')
        {
            state = internal_state::expecting_newline_2;
            return RequestStatus::indeterminate;
        }
        if (input == ' ' || input == '\t')
        {
            return RequestStatus::indeterminate;
        }
        if (is_CTL(input))
        {
            return RequestStatus::invalid;
        }
        state = internal_state::header_value;
        return RequestStatus::indeterminate;
    case internal_state::header_name:
        if (input == ':')
        {
            state = internal_state::space_before_header_value;
            return RequestStatus::indeterminate;
        }
        if (!is_char(input) || is_CTL(input) || is_special(input))
        {
            return RequestStatus::invalid;
        }
        current_header.name.push_back(input);
        return RequestStatus::indeterminate;
    case internal_state::space_before_header_value:
        if (input == ' ')
        {
            state = internal_state::header_value;
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    case internal_state::header_value:
        if (input == '\r')
        {
            state = internal_state::expecting_newline_2;
            return RequestStatus::indeterminate;
        }
        if (is_CTL(input))
        {
            return RequestStatus::invalid;
        }
        current_header.value.push_back(input);
        return RequestStatus::indeterminate;
    case internal_state::expecting_newline_2:
        if (input == '\n')
        {
            state = internal_state::header_line_start;
            return RequestStatus::indeterminate;
        }
        return RequestStatus::invalid;
    default: // expecting_newline_3
        return input == '\n' ? RequestStatus::valid : RequestStatus::invalid;
    }
}

bool RequestParser::is_char(const int character) const
{
    return character >= 0 && character <= 127;
}

bool RequestParser::is_CTL(const int character) const
{
    return (character >= 0 && character <= 31) || (character == 127);
}

bool RequestParser::is_special(const int character) const
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

bool RequestParser::is_digit(const int character) const
{
    return character >= '0' && character <= '9';
}
}
}
