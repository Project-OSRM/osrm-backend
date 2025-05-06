#include "extractor/area/area_manager.hpp"

#include "util/log.hpp"

#include <osmium/area/assembler.hpp>

#include <algorithm>

namespace osrm::extractor::area
{

/**
 * @brief Registers the given closed way for area-building.
 *
 * This function is thread-safe.
 *
 * This member function is named way() so the manager can
 * be used as a handler for the first pass through a data file.
 *
 * @param way Way we want to build.
 */
void AreaManager::way(const osmium::Way &way)
{
    // the way must be closed
    if (!way.ends_have_same_id())
    {
        return;
    }
    // the way must have at least 4 nodes (first and last are the same)
    if (way.nodes().size() < 4)
    {
        return;
    }
    // the mapper said "no"
    if (way.tags().has_tag("area", "no"))
    {
        return;
    }
    // ok
    const char *name = way.get_value_by_key("name", "noname");
    util::Log(logINFO) << "Registering way: " << name << " id: " << way.id();
    registered_closed_ways.push_back(way.id());
    ++number_of_ways;
}

/**
 * @brief Registers the given relation for area-building.
 *
 * This function is thread-safe.
 *
 * We are interested only in relations tagged with type=multipolygon
 * with at least one way member.
 *
 * This member function is named relation() so the manager can
 * be used as a handler for the first pass through a data file.
 *
 * @param relation Relation we want to build.
 */
void AreaManager::relation(const osmium::Relation &relation)
{
    const char *type = relation.tags().get_value_by_key("type");

    // the relation must be a multipolygon
    if (type == nullptr || std::strcmp(type, "multipolygon") != 0)
    {
        return;
    }

    // the relation must have at least one way member
    if (!std::any_of(relation.members().cbegin(),
                     relation.members().cend(),
                     [](const osmium::RelationMember &member)
                     { return member.type() == osmium::item_type::way; }))
    {
        return;
    }

    const char *name = relation.get_value_by_key("name", "noname");
    util::Log(logINFO) << "Registering relation: " << name << " id: " << relation.id();

    // osmium is not thread-safe
    MutexType::scoped_lock lock(mutex);
    auto rel_handle = relations_database().add(relation);

    std::size_t n = 0;
    for (auto &member : rel_handle->members())
    {
        if (member.type() == osmium::item_type::way)
        {
            member_database(member.type()).track(rel_handle, member.ref(), n);
            m_ring_of[member.ref()] = relation.id();
            util::Log(logINFO) << "  Ring: id: " << member.ref();
        }
        else
        {
            member.set_ref(0); // set member id to zero to indicate we are not interested
        }
        ++n;
    }
    ++number_of_relations;
}

/**
 * Sort the members databases to prepare them for reading. Usually
 * this is called between the first and second pass reading through
 * an OSM data file.
 */
void AreaManager::prepare_for_lookup(extractor::NodeLocationsForWays &nlw)
{
    nlw.prepare_for_lookup();
    node_locations_for_ways = &nlw;
    RelationsManager::prepare_for_lookup();
    std::sort(registered_closed_ways.begin(), registered_closed_ways.end());
}

inline bool AreaManager::is_registered_closed_way(osmium::object_id_type osm_id) const
{
    return std::binary_search(registered_closed_ways.begin(), registered_closed_ways.end(), osm_id);
}

ExtractionRelationContainer::RelationIDList AreaManager::get_relations(const osmium::Way &way) const
{
    ExtractionRelationContainer::RelationIDList result;
    auto found = m_ring_of.find(way.id());
    if (found != m_ring_of.end())
        result.push_back(
            ExtractionRelation::OsmIDTyped{found->second, osmium::item_type::relation});
    return result;
}

/**
 * Second-pass handler for ways.
 */
void AreaManager::after_way(const osmium::Way &way)
{
    if (is_registered_closed_way(way.id()))
    {
        const char *name = way.get_value_by_key("name", "noname");
        util::Log(logINFO) << "Completing way: " << name << " id: " << way.id();

        node_locations_for_ways->way(way);
        osmium::area::Assembler assembler{m_assembler_config};
        assembler(way, this->buffer());
    }
}

/**
 * Second-pass handler for relations.
 *
 * This is called when a relation is complete, ie. all members
 * were found in the input.
 */
void AreaManager::complete_relation(const osmium::Relation &relation)
{
    const char *name = relation.get_value_by_key("name", "noname");
    util::Log(logINFO) << "Completing relation: " << name << " id: " << relation.id();

    std::vector<const osmium::Way *> ways;
    ways.reserve(relation.members().size());
    for (const auto &member : relation.members())
    {
        if (member.ref() != 0)
        {
            const osmium::Way *way = get_member_way(member.ref());
            assert(way != nullptr);
            node_locations_for_ways->way(*way);
            ways.push_back(way);
        }
    }

    try
    {
        osmium::area::Assembler assembler{m_assembler_config};
        assembler(relation, ways, this->buffer());
    }
    catch (const osmium::invalid_location &)
    {
        // XXX ignore
    }
}

} // namespace osrm::extractor::area
