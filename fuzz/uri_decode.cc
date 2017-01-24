#include "server/api/url_parser.hpp"
#include "util/string_util.hpp"

#include "util.hpp"

#include <iterator>
#include <string>

using osrm::util::URIDecode;

extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data, unsigned long size)
{
    const std::string in(reinterpret_cast<const char *>(data), size);
    std::string out;

    (void)URIDecode(in, out);

    escape(out.data());

    return 0;
}
