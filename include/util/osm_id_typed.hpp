#ifndef OSM_ID_TYPED_HPP
#define OSM_ID_TYPED_HPP

#include "typedefs.hpp"

namespace osrm
{
namespace util
{

class OsmIDTyped
{
public:
    using HashType = std::uint64_t;

    OsmIDTyped(std::uint64_t id_, std::uint8_t type_)
        : id(id_)
        , type(type_)
    {
        // check if type value not above type size bound
        BOOST_ASSERT(id_ < (std::uint64_t(1) << 56));
    }

    bool operator == (const OsmIDTyped &other) { return (id == other.id && type == other.type); }
    bool operator != (const OsmIDTyped &other) { return (id != other.id || type != other.type); }

    inline HashType Hash() const { return (std::uint64_t(id) | std::uint64_t(type) << 56); }

    std::uint64_t GetID() const { return id; }
    std::uint8_t GetType() const { return type; }

private:
    std::uint64_t id : 56;
    std::uint8_t type;
};

} // namespace util
} // namespace osrm

#endif
