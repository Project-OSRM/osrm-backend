#ifndef EXTRACTION_RELATION_HPP
#define EXTRACTION_RELATION_HPP

#include "util/exception.hpp"

#include <boost/assert.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/flyweight.hpp>
#include <boost/flyweight/flyweight_fwd.hpp>
#include <boost/flyweight/no_tracking.hpp>

#include <oneapi/tbb/concurrent_map.h>
#include <oneapi/tbb/concurrent_vector.h>

#include <osmium/osm/area.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/object.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/osm/way.hpp>

#include <map>
#include <string>
#include <vector>

namespace osrm::extractor
{

namespace
{
using string = boost::flyweights::flyweight<std::string, boost::flyweights::no_tracking>;
using rel_id_type = osmium::object_id_type;
using member_id_type = osmium::object_id_type;
} // namespace

/**
 * @brief A replacement for `osmium::RelationMember`
 *
 * A replacement is needed because `osmium::RelationMember` has has no copy-constructor,
 * but SOL/LUA wants a copy-constructor.
 *
 * Care has been taken to minimize memory consumption.  This implementation uses a
 * flyweight-pattern to store the very repetitive OSM member role.
 */
class RelationMember
{
    string m_role;
    member_id_type m_ref;
    osmium::item_type m_type;

  public:
    RelationMember(member_id_type id, osmium::item_type type) : m_ref{id}, m_type{type} {};
    RelationMember(const osmium::RelationMember &o)
        : m_role(o.role()), m_ref{o.ref()}, m_type{o.type()} {};

    member_id_type ref() const noexcept { return m_ref; }
    osmium::item_type type() const noexcept { return m_type; }
    const string &role() const noexcept { return m_role; }

    friend bool operator==(const RelationMember &m, const RelationMember &o)
    {
        return m.ref() == o.ref() && m.type() == o.type();
    }
    friend bool operator<(const RelationMember &m, const RelationMember &o)
    {
        return m.ref() < o.ref() || (m.ref() == o.ref() && m.type() < o.type());
    }
};

/**
 * @brief A replacement for `osmium::Relation`
 *
 * A replacement is needed because in-memory storage for `osmium::Relation` in an
 * `osmium::Stash` is expensive.
 *
 * Care has been taken to minimize memory consumption.  This implementation uses a
 * flyweight-pattern to store the very repetitive OSM tags.
 *
 */
class Relation
{
    using members_t = std::vector<RelationMember>;

    std::map<string, string> m_tags;
    members_t m_members;
    member_id_type m_id;
    osmium::object_version_type m_version;
    bool sorted{false};

  public:
    /**
     * @brief Construct a new `Relation` object from an `osmium::Relation`
     *
     * @param o The `osmium::Relation`
     */
    Relation(const osmium::Relation &o) : m_id{o.id()}, m_version(o.version())
    {
        for (const osmium::Tag &tag : o.tags())
        {
            m_tags.emplace(tag.key(), tag.value());
        }
        for (const osmium::RelationMember &member : o.cmembers())
        {
            m_members.emplace_back(member);
        }
    };

    member_id_type id() const noexcept { return m_id; };
    osmium::item_type type() const noexcept { return osmium::item_type::relation; };
    osmium::object_version_type version() const noexcept { return m_version; };

    /**
     * @brief Return the tag value for the given key or the given default.
     *
     * @param key The key
     * @param default_value The default value
     * @return const char* The tag value, or the given default value if the tag does not
     * exist
     */
    const char *get_value_by_key_default(const char *key, const char *default_value) const
    {
        // Nitpick: if somebody searches for "higway", it will be added to the flyweights
        auto found = m_tags.find(string(key));
        if (found != m_tags.end())
            return found->second.get().c_str();
        return default_value;
    };

    /**
     * @brief Return the tag value for the given key or `nullptr`.
     *
     * @param key The tag key
     * @return const char* The tag value, or nullptr if the tag does not exist
     */
    const char *get_value_by_key(const char *key) const
    {
        return get_value_by_key_default(key, nullptr);
    }

    /**
     * @brief Return the role that the given object has in the relation
     *
     * @tparam T Any type that has T.id() and T.type()
     * @param o The relation member
     * @return const char* The role, eg. "outer"
     */
    template <class T> const char *get_member_role(const T &o)
    {
        return get_member_role(RelationMember(o.id(), o.type()));
    }

    const char *get_member_role(const RelationMember &m);
    const members_t &members() { return m_members; };
};

/**
 * @brief A storage container for Relation objects
 *
 * This is the object passed to the @ref process_node, @ref process_way, and @ref
 * process_relation functions as the `relations` argument. It answers the question:
 * Which relations do contain this OSM object?
 *
 * To save on memory this object stores only the relation types specified in
 * @ref relation_types and the multipolygon relations registered for meshing.
 */
class ExtractionRelationContainer
{
    using rel_ids_t = tbb::concurrent_vector<rel_id_type>;
    using parent_map_t = tbb::concurrent_map<member_id_type, rel_ids_t>;

    tbb::concurrent_map<rel_id_type, Relation> relations;
    std::vector<parent_map_t> parents;

    static rel_ids_t empty_rel_list;

    parent_map_t &p(osmium::item_type t) { return parents[(size_t)t]; }
    const parent_map_t &p(osmium::item_type t) const { return parents[(size_t)t]; }

  public:
    ExtractionRelationContainer(ExtractionRelationContainer &&) = delete;
    ExtractionRelationContainer(const ExtractionRelationContainer &) = delete;
    ExtractionRelationContainer() : parents(4){};

    /**
     * @brief Add a relation to the container
     *
     * Add a relation to the container and register all its members.
     *
     * @param rel The relation to add
     */
    void add_relation(const osmium::Relation &rel)
    {
        if (relations.contains(rel.id()))
            return;
        for (auto const &m : rel.members())
        {
            add_relation_member(rel.id(), m.ref(), m.type());
        }
        relations.emplace(rel.id(), rel);
    }

    /**
     * @brief Register a member of the given relation
     *
     * @param relation_id The id of the relation
     * @param member_id The id of the member
     * @param member_type The type of the member
     */
    void add_relation_member(rel_id_type relation_id,
                             member_id_type member_id,
                             osmium::item_type member_type)
    {
        p(member_type)[member_id].push_back(relation_id);
    }

    /**
     * @brief Return the number of relations in the container
     */
    std::size_t get_relations_num() const { return relations.size(); }

    const rel_ids_t &_get_relations_for(member_id_type member_id,
                                        osmium::item_type member_type) const;

    /**
     * @brief Return the relations that contain the given OSM object.
     *
     * @param member The OSM object
     * @return The ids of the relations
     */
    template <class T> const rel_ids_t &get_relations_for(const T &member) const
    {
        return _get_relations_for(member.id(), member.type());
    }

    /**
     * @brief Get the Relation object for the given relation id
     *
     * @param rel_id The relation id
     * @return The relation object
     */
    const Relation &get_relation(rel_id_type rel_id) const
    {
        auto it = relations.find(rel_id);
        if (it == relations.end())
            throw osrm::util::exception("Can't find relation data for " + std::to_string(rel_id));

        return it->second;
    }
};

const char *get_osmium_member_role(const osmium::Relation &rel, const osmium::OSMObject &o);
std::vector<RelationMember> get_osmium_relation_members(const osmium::Relation &rel);

} // namespace osrm::extractor

#endif // EXTRACTION_RELATION_HPP
