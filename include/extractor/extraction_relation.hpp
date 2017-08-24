#ifndef EXTRACTION_RELATION_HPP
#define EXTRACTION_RELATION_HPP

#include "util/osm_id_typed.hpp"

#include <string>
#include <unordered_map>

namespace osrm
{
namespace extractor
{
namespace detail
{

inline const char * checkedString(const char * str)
{
    return str ? str : "";
}

} // namespace detail

struct ExtractionRelation
{
    using AttributesMap = std::unordered_map<std::string, std::string>;

    ExtractionRelation()
        : is_restriction(false)
    {
    }

    void clear()
    {
        is_restriction = false;
        values.clear();
    }

    bool IsRestriction() const
    {
        return is_restriction;
    }

    AttributesMap & GetMember(util::OsmIDTyped id)
    {
        return values[id.Hash()];
    }

//    AttributesMap & operator[] (util::OsmIDTyped id)
//    {
//        return values[id];
//    }

    bool is_restriction;
    std::unordered_map<util::OsmIDTyped::HashType, AttributesMap> values;
};

} // namespace extractor
} // namespace osrm

#endif // EXTRACTION_RELATION_HPP
