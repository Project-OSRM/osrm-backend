#ifndef EXTRACTION_RELATION_HPP
#define EXTRACTION_RELATION_HPP

#include "util/osm_id_typed.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace osrm
{
namespace extractor
{
namespace detail
{

inline const char *checkedString(const char *str) { return str ? str : ""; }

} // namespace detail

struct ExtractionRelation
{
    using AttributesMap = std::unordered_map<std::string, std::string>;

    ExtractionRelation() : is_restriction(false) {}

    void clear()
    {
        is_restriction = false;
        values.clear();
    }

    bool IsRestriction() const { return is_restriction; }

    AttributesMap &GetMember(util::OsmIDTyped id) { return values[id.Hash()]; }

    bool is_restriction;
    std::unordered_map<util::OsmIDTyped::HashType, AttributesMap> values;
};

// It contains data of all parsed relations for each node/way element
class ExtractionRelationContainer
{
  public:
    using AttributesMap = ExtractionRelation::AttributesMap;
    using RelationList = std::vector<AttributesMap>;

    void AddRelation(const ExtractionRelation &rel)
    {
        BOOST_ASSERT(!rel.is_restriction);
        for (auto it : rel.values)
            data[it.first].push_back(it.second);
    }

    const RelationList &Get(const util::OsmIDTyped &id) const
    {
        const auto it = data.find(id.Hash());
        if (it != data.end())
            return it->second;

        static RelationList empty;
        return empty;
    }

  private:
    // TODO: need to store more common data
    std::unordered_map<util::OsmIDTyped::HashType, RelationList> data;
};

} // namespace extractor
} // namespace osrm

#endif // EXTRACTION_RELATION_HPP
