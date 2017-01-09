#include "engine/hint.hpp"
#include "engine/base64.hpp"
#include "engine/datafacade/datafacade_base.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <iterator>
#include <ostream>
#include <tuple>

namespace osrm
{
namespace engine
{

bool Hint::IsValid(const util::Coordinate new_input_coordinates,
                   const datafacade::BaseDataFacade &facade) const
{
    auto is_same_input_coordinate = new_input_coordinates.lon == phantom.input_location.lon &&
                                    new_input_coordinates.lat == phantom.input_location.lat;
    // FIXME this does not use the number of nodes to validate the phantom because
    // GetNumberOfNodes()
    // depends on the graph which is algorithm dependent
    return is_same_input_coordinate && phantom.IsValid() && facade.GetCheckSum() == data_checksum;
}

std::string Hint::ToBase64() const
{
    auto base64 = encodeBase64Bytewise(*this);

    // Make safe for usage as GET parameter in URLs
    std::replace(begin(base64), end(base64), '+', '-');
    std::replace(begin(base64), end(base64), '/', '_');

    return base64;
}

Hint Hint::FromBase64(const std::string &base64Hint)
{
    BOOST_ASSERT_MSG(base64Hint.size() == ENCODED_HINT_SIZE, "Hint has invalid size");

    // We need mutability but don't want to change the API
    auto encoded = base64Hint;

    // Reverses above encoding we need for GET parameters in URL
    std::replace(begin(encoded), end(encoded), '-', '+');
    std::replace(begin(encoded), end(encoded), '_', '/');

    return decodeBase64Bytewise<Hint>(encoded);
}

bool operator==(const Hint &lhs, const Hint &rhs)
{
    return std::tie(lhs.phantom, lhs.data_checksum) == std::tie(rhs.phantom, rhs.data_checksum);
}

std::ostream &operator<<(std::ostream &out, const Hint &hint) { return out << hint.ToBase64(); }

} // ns engine
} // ns osrm
