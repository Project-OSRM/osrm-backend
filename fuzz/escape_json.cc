#include "util/string_util.hpp"

#include "util.hpp"

#include <iterator>
#include <string>

using osrm::util::escape_JSON;

extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data, unsigned long size)
{
    const std::string in(reinterpret_cast<const char *>(data), size);

    const auto escaped = escape_JSON(in);
    escape(escaped.data());

    return 0;
}
