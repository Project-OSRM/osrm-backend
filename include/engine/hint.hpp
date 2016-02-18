#ifndef ENGINE_HINT_HPP
#define ENGINE_HINT_HPP

#include "engine/object_encoder.hpp"
#include "engine/phantom_node.hpp"

#include <boost/assert.hpp>

#include <cstdint>
#include <cmath>

#include <string>

namespace osrm
{
namespace engine
{

// Is returned as a temporary identifier for snapped coodinates
struct Hint
{
    FixedPointCoordinate input_coordinate;
    PhantomNode phantom;
    std::uint32_t data_checksum;

    template <typename DataFacadeT>
    bool IsValid(const FixedPointCoordinate new_input_coordinates, DataFacadeT &facade) const
    {
        auto is_same_input_coordinate = new_input_coordinates.lat == input_coordinate.lat &&
                                        new_input_coordinates.lon == input_coordinate.lon;
        return is_same_input_coordinate && phantom.IsValid(facade.GetNumberOfNodes()) &&
               facade.GetCheckSum() == data_checksum;
    }

    std::string ToBase64() const { return encodeBase64(*this); }

    static Hint FromBase64(const std::string &base64Hint)
    {
        BOOST_ASSERT_MSG(base64Hint.size() ==
                             static_cast<std::size_t>(std::ceil(sizeof(Hint) / 3.) * 4),
                         "Hint has invalid size");

        auto decoded = decodeBase64<Hint>(base64Hint);
        return decoded;
    }
};

#ifndef _MSC_VER
constexpr std::size_t ENCODED_HINT_SIZE = 88;
static_assert(ENCODED_HINT_SIZE / 4 * 3 >= sizeof(Hint),
              "ENCODED_HINT_SIZE does not match size of Hint");
#else
// PhantomNode is bigger under windows because MSVC does not support bit packing
constexpr std::size_t ENCODED_HINT_SIZE = 84;
static_assert(ENCODED_HINT_SIZE / 4 * 3 >= sizeof(Hint),
              "ENCODED_HINT_SIZE does not match size of Hint");
#endif
}
}

#endif
