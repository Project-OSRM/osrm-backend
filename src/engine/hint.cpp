#include "engine/hint.hpp"
#include "engine/base64.hpp"
#include "engine/datafacade/datafacade_base.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{

bool Hint::IsValid(const util::Coordinate new_input_coordinates,
                   const datafacade::BaseDataFacade &facade) const
{
    auto is_same_input_coordinate = new_input_coordinates.lon == phantom.input_location.lon &&
                                    new_input_coordinates.lat == phantom.input_location.lat;
    return is_same_input_coordinate && phantom.IsValid(facade.GetNumberOfNodes()) &&
           facade.GetCheckSum() == data_checksum;
}

std::string Hint::ToBase64() const { return encodeBase64Bytewise(*this); }

Hint Hint::FromBase64(const std::string &base64Hint)
{
    BOOST_ASSERT_MSG(base64Hint.size() == ENCODED_HINT_SIZE, "Hint has invalid size");

    return decodeBase64Bytewise<Hint>(base64Hint);
}
}
}
