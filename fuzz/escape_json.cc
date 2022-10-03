#include "util/string_util.hpp"

#include "util.hpp"

#include <iterator>
#include <string>

using osrm::util::EscapeJSONString;

extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data, unsigned long size)
{
    const std::string in(reinterpret_cast<const char *>(data), size);

    std::string escaped;
    EscapeJSONString(in, escaped);
    escape(escaped.data());

    return 0;
}
