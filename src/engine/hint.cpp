#include "engine/hint.hpp"
#include "engine/object_encoder.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{
std::string Hint::ToBase64() const { return encodeBase64(*this); }

Hint Hint::FromBase64(const std::string &base64Hint)
{
    BOOST_ASSERT_MSG(base64Hint.size() == ENCODED_HINT_SIZE, "Hint has invalid size");

    auto decoded = decodeBase64<Hint>(base64Hint);
    return decoded;
}
}
}
