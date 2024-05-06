#ifndef EXTRACTION_RELATION_HPP
#define EXTRACTION_RELATION_HPP

#include "util/exception.hpp"

#include <osmium/osm/relation.hpp>

#include <boost/assert.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace osrm::extractor
{

struct ExtractionRelation
{
    class OsmIDTyped
    {
      public:
        OsmIDTyped(osmium::object_id_type _id, osmium::item_type _type) : id(_id), type(_type) {}

        std::uint64_t GetID() const { return std::uint64_t(id); }
        osmium::item_type GetType() const { return type; }

        std::uint64_t Hash() const { return id ^ (static_cast<std::uint64_t>(type) << 56); }

      private:
        osmium::object_id_type id;
        osmium::item_type type;
    };

    using AttributesList = std::vector<std::pair<std::string, std::string>>;
    using MembersRolesList = std::vector<std::pair<std::uint64_t, std::string>>;

    explicit ExtractionRelation(const OsmIDTyped &_id) : id(_id) {}

    void Clear()
    {
        attributes.clear();
        members_role.clear();
    }

    const char *GetAttr(const std::string &attr) const
    {
        auto it = std::lower_bound(
            attributes.begin(), attributes.end(), std::make_pair(attr, std::string()));

        if (it != attributes.end() && (*it).first == attr)
            return (*it).second.c_str();

        return nullptr;
    }

    void Prepare()
    {
        std::sort(attributes.begin(), attributes.end());
        std::sort(members_role.begin(), members_role.end());
    }

    void AddMember(const OsmIDTyped &member_id, const char *role)
    {
        members_role.emplace_back(std::make_pair(member_id.Hash(), std::string(role)));
    }

    const char *GetRole(const OsmIDTyped &member_id) const
    {
        const auto hash = member_id.Hash();
        auto it = std::lower_bound(
            members_role.begin(), members_role.end(), std::make_pair(hash, std::string()));

        if (it != members_role.end() && (*it).first == hash)
            return (*it).second.c_str();

        return nullptr;
    }

    OsmIDTyped id;
    AttributesList attributes;
    MembersRolesList members_role;
};

// It contains data of all parsed relations for each node/way element
class ExtractionRelationContainer
{
  public:
    using AttributesMap = ExtractionRelation::AttributesList;
    using OsmIDTyped = ExtractionRelation::OsmIDTyped;
    using RelationList = std::vector<AttributesMap>;
    using RelationIDList = std::vector<ExtractionRelation::OsmIDTyped>;
    using RelationRefMap = std::unordered_map<std::uint64_t, RelationIDList>;

    ExtractionRelationContainer() = default;
    ExtractionRelationContainer(ExtractionRelationContainer &&) = default;
    ExtractionRelationContainer(const ExtractionRelationContainer &) = delete;

    void AddRelation(ExtractionRelation &&rel)
    {
        rel.Prepare();

        BOOST_ASSERT(relations_data.find(rel.id.GetID()) == relations_data.end());
        relations_data.insert(std::make_pair(rel.id.GetID(), std::move(rel)));
    }

    void AddRelationMember(const OsmIDTyped &relation_id, const OsmIDTyped &member_id)
    {
        switch (member_id.GetType())
        {
        case osmium::item_type::node:
            node_refs[member_id.GetID()].push_back(relation_id);
            break;

        case osmium::item_type::way:
            way_refs[member_id.GetID()].push_back(relation_id);
            break;

        case osmium::item_type::relation:
            rel_refs[member_id.GetID()].push_back(relation_id);
            break;

        default:
            break;
        };
    }

    void Merge(ExtractionRelationContainer &&other)
    {
        for (auto it : other.relations_data)
        {
            const auto res = relations_data.insert(std::make_pair(it.first, std::move(it.second)));
            BOOST_ASSERT(res.second);
            (void)res; // prevent unused warning in release
        }

        auto MergeRefMap = [&](RelationRefMap &source, RelationRefMap &target)
        {
            for (auto it : source)
            {
                auto &v = target[it.first];
                v.insert(v.end(), it.second.begin(), it.second.end());
            }
        };

        MergeRefMap(other.way_refs, way_refs);
        MergeRefMap(other.node_refs, node_refs);
        MergeRefMap(other.rel_refs, rel_refs);
    }

    std::size_t GetRelationsNum() const { return relations_data.size(); }

    const RelationIDList &GetRelations(const OsmIDTyped &member_id) const
    {
        auto getFromMap = [this](std::uint64_t id,
                                 const RelationRefMap &map) -> const RelationIDList &
        {
            auto it = map.find(id);
            if (it != map.end())
                return it->second;

            return empty_rel_list;
        };

        switch (member_id.GetType())
        {
        case osmium::item_type::node:
            return getFromMap(member_id.GetID(), node_refs);

        case osmium::item_type::way:
            return getFromMap(member_id.GetID(), way_refs);

        case osmium::item_type::relation:
            return getFromMap(member_id.GetID(), rel_refs);

        default:
            break;
        }

        return empty_rel_list;
    }

    const ExtractionRelation &GetRelationData(const ExtractionRelation::OsmIDTyped &rel_id) const
    {
        auto it = relations_data.find(rel_id.GetID());
        if (it == relations_data.end())
            throw osrm::util::exception("Can't find relation data for " +
                                        std::to_string(rel_id.GetID()));

        return it->second;
    }

  private:
    RelationIDList empty_rel_list;
    std::unordered_map<std::uint64_t, ExtractionRelation> relations_data;

    // each map contains list of relation id's, that has keyed id as a member
    RelationRefMap way_refs;
    RelationRefMap node_refs;
    RelationRefMap rel_refs;
};

} // namespace osrm::extractor

#endif // EXTRACTION_RELATION_HPP
