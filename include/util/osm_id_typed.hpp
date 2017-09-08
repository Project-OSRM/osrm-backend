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
    enum class Type : uint8_t
    {
        Unknown = 0,
        Node,
        Way,
        Relation
    };

    using HashType = std::uint64_t;

    OsmIDTyped(std::uint64_t id_, Type type_) : id(id_), type(type_)
    {
        // check if type value not above type size bound
        BOOST_ASSERT(id_ < (std::uint64_t(1) << 56));
    }

    bool operator==(const OsmIDTyped &other) { return (id == other.id && type == other.type); }
    bool operator!=(const OsmIDTyped &other) { return (id != other.id || type != other.type); }

    inline HashType Hash() const { return (std::uint64_t(id) | std::uint64_t(type) << 56); }

    std::uint64_t GetID() const { return id; }
    Type GetType() const { return type; }

  private:
    std::uint64_t id : 56;
    Type type;
};

} // namespace util
} // namespace osrm

#endif
