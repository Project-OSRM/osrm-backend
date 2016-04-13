#include "server/api/url_parser.hpp"

#include "util.hpp"

#include <iterator>
#include <string>

using osrm::server::api::parseURL;

extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data, unsigned long size)
{
    std::string in(reinterpret_cast<const char *>(data), size);

    auto first = begin(in);
    const auto last = end(in);

    const auto param = parseURL(first, last);
    escape(&param);

    return 0;
}
