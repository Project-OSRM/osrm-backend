#ifndef EXTRACTION_RELATION_HPP
#define EXTRACTION_RELATION_HPP

#include <osmium/osm/relation.hpp>

#include <boost/assert.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace osrm
{
namespace extractor
{

struct ExtractionRelation
{
    using AttributesMap = std::unordered_map<std::string, std::string>;
    using OsmIDTyped = std::pair<osmium::object_id_type, osmium::item_type>;

    struct OsmIDTypedHash
    {
        std::size_t operator()(const OsmIDTyped &id) const
        {
            return id.first ^ (static_cast<std::uint64_t>(id.second) << 56);
        }
    };

    ExtractionRelation() : is_restriction(false) {}

    void clear()
    {
        is_restriction = false;
        values.clear();
    }

    bool IsRestriction() const { return is_restriction; }

    AttributesMap &GetMember(const osmium::RelationMember &member)
    {
        return values[OsmIDTyped(member.ref(), member.type())];
    }

    bool is_restriction;
    std::unordered_map<OsmIDTyped, AttributesMap, OsmIDTypedHash> values;
};

// It contains data of all parsed relations for each node/way element
class ExtractionRelationContainer
{
  public:
    using AttributesMap = ExtractionRelation::AttributesMap;
    using OsmIDTyped = ExtractionRelation::OsmIDTyped;
    using RelationList = std::vector<AttributesMap>;

    void AddRelation(const ExtractionRelation &rel)
    {
        BOOST_ASSERT(!rel.is_restriction);
        for (auto it : rel.values)
            data[it.first].push_back(it.second);
    }

    const RelationList &Get(const OsmIDTyped &id) const
    {
        const auto it = data.find(id);
        if (it != data.end())
            return it->second;

        static RelationList empty;
        return empty;
    }

  private:
    std::unordered_map<OsmIDTyped, RelationList, ExtractionRelation::OsmIDTypedHash> data;
};

} // namespace extractor
} // namespace osrm

#endif // EXTRACTION_RELATION_HPP
