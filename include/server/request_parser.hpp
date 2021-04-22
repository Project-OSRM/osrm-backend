#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include "server/http/compression_type.hpp"
#include "server/http/header.hpp"

#include <tuple>

namespace osrm
{
namespace server
{

namespace http
{
struct request;
}

class RequestParser
{
  public:
    RequestParser();

    enum class RequestStatus : char
    {
        valid,
        invalid,
        indeterminate
    };

    std::tuple<RequestStatus, http::compression_type>
    parse(http::request &current_request, char *begin, char *end);

  private:
    RequestStatus consume(http::request &current_request, const char input);

    bool is_char(const int character) const;

    bool is_CTL(const int character) const;

    bool is_special(const int character) const;

    bool is_digit(const int character) const;

    enum class internal_state : unsigned char
    {
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
        header_value,
        expecting_newline_2,
        expecting_newline_3
    } state;

    http::header current_header;
    http::compression_type selected_compression;
};
} // namespace server
} // namespace osrm

#endif // REQUEST_PARSER_HPP
