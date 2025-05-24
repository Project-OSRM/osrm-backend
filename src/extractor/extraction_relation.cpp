#include "extractor/extraction_relation.hpp"

namespace osrm::extractor
{

/**
 * @brief Return the role of the given object in the given relation
 *
 * @param rel The relation
 * @param o The OSM object
 * @return The role or nullptr
 */
const char *get_osmium_member_role(const osmium::Relation &rel, const osmium::OSMObject &o)
{
    for (const auto &member : rel.cmembers())
    {
        if (member.ref() == o.id() && member.type() == o.type())
            return member.role();
    }
    return nullptr;
};

/**
 * @brief Return the members of a relation as `RelationMember`s
 *
 * This is a helper function for SOL/LUA: `osmium::RelationMember` is not
 * copy-constructable. But SOL/LUA wants to copy the members. We turn the
 * `osmium::RelationMember` into a copy-constructable `RelationMember`.
 *
 * @param rel The `osmium::Relation`
 * @return The members as `RelationMember`
 */
std::vector<RelationMember> get_osmium_relation_members(const osmium::Relation &rel)
{
    std::vector<RelationMember> result;
    for (const auto &member : rel.cmembers())
    {
        result.push_back(RelationMember(member));
    }
    return result;
};

const char *Relation::get_member_role(const RelationMember &m)
{
    if (!sorted)
    {
        std::sort(m_members.begin(), m_members.end());
        sorted = true;
    }
    auto it = std::lower_bound(m_members.begin(), m_members.end(), m);
    if (it != m_members.end() && *it == m)
    {
        return it->role().get().c_str();
    }
    return nullptr;
}

ExtractionRelationContainer::rel_ids_t ExtractionRelationContainer::empty_rel_list{};

/**
 * @brief Return the relations which contain the given member
 *
 * @param member_id The id of the member
 * @param member_type The type of the member
 * @return The ids of the relations
 */
const ExtractionRelationContainer::rel_ids_t &
ExtractionRelationContainer::_get_relations_for(member_id_type member_id,
                                                osmium::item_type member_type) const
{
    if (member_type == osmium::item_type::area)
    {
        member_type = (member_id & 1) ? osmium::item_type::relation : osmium::item_type::way;
        member_id /= 2;
    }
    assert(member_type == osmium::item_type::relation || member_type == osmium::item_type::way ||
           member_type == osmium::item_type::node);

    const parent_map_t &parents = p(member_type);

    auto it = parents.find(member_id);
    if (it != parents.end())
        return it->second;

    return empty_rel_list;
}

} // namespace osrm::extractor
