#include "server/request_parser.hpp"
#include "server/http/request.hpp"

#include "util.hpp"

#include <iterator>
#include <string>

using osrm::server::RequestParser;
using osrm::server::http::request;

extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data, unsigned long size)
{
    std::string in(reinterpret_cast<const char *>(data), size);

    auto first = begin(in);
    auto last = end(in);

    RequestParser parser;
    request req;

    // &(*it) is needed to go from iterator to underlying item to pointer to underlying item
    parser.parse(req, &(*first), &(*last));

    escape(&req);

    return 0;
}
