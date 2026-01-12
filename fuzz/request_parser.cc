#include "util.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/beast/http.hpp>

#include <iterator>
#include <string>

namespace beast = boost::beast;
namespace http_proto = beast::http;

extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data, unsigned long size)
{
    std::string in(reinterpret_cast<const char *>(data), size);

    http_proto::request_parser<http_proto::empty_body> parser;
    parser.eager(true);

    beast::error_code ec;
    parser.put(boost::asio::buffer(in.data(), in.size()), ec);
    if (!ec)
    {
        parser.put_eof(ec);
    }

    // Make the parsed state observable to the optimizer
    if (!ec && parser.is_done())
    {
        auto &req = parser.get();
        escape(&req);
    }

    return 0;
}
