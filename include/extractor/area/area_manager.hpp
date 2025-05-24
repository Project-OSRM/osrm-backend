#ifndef OSRM_EXTRACTOR_AREA_AREA_MANAGER_HPP
#define OSRM_EXTRACTOR_AREA_AREA_MANAGER_HPP

#include "extractor/extraction_relation.hpp"
#include <oneapi/tbb/concurrent_map.h>
#include <oneapi/tbb/concurrent_set.h>
#include <oneapi/tbb/concurrent_vector.h>
#include <oneapi/tbb/mutex.h>
#include <osmium/area/assembler_config.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/relations/relations_database.hpp>
#include <osmium/relations/relations_manager.hpp>

namespace osrm::extractor::area
{

/**
 * @brief A registry for area objects.
 *
 * This class backs the `area_manager` global variable in all LUA scripts. Call
 * AreaManager::way() if you want to turn a closed way into an area, call
 * AreaMananger::relation() if you want to turn a multipolygon relation into an area.
 */
class AreaManager
    : public osmium::relations::RelationsManager<AreaManager, false, true, false, false>
{
    osmium::area::AssemblerConfig m_assembler_config;
    using MutexType = tbb::mutex;
    /** Mutex to protect relations_manager */
    MutexType mutex;

    using relation_ids = std::vector<osmium::object_id_type>;
    std::string algorithm_name;
    bool enabled{false};

  public:
    AreaManager(ExtractionRelationContainer &c) : relations_stash{c}
    {
        m_assembler_config.debug_level = 0;
    };

    void init(const char *algorithm_name);
    void way(const osmium::Way &way);
    void relation(const osmium::Relation &relation);
    void prepare_for_lookup();
    bool is_registered_closed_way(osmium::object_id_type way_id) const;
    relation_ids get_relations_for_node(const osmium::Node &) const;
    relation_ids get_relations_for_way(const osmium::Way &) const;

    void complete_relation(const osmium::Relation &relation);
    void after_way(const osmium::Way &way);
    bool is_enabled() { return enabled; }

    std::size_t number_of_ways{0};
    std::size_t number_of_relations{0};

    /**
     * This collects the closed ways and the outer ring members of the relations we have
     * registered for meshing. They are also used for collecting the intersecting ways.
     */
    tbb::concurrent_vector<osmium::object_id_type> registered_closed_ways;
    /** Map of way_id -> rel_id: if way is a member of rel */
    tbb::concurrent_map<osmium::object_id_type, osmium::object_id_type> m_way_relation;
    /** Map of node_id -> rel_id: if node is a member of rel */
    tbb::concurrent_map<osmium::object_id_type, osmium::object_id_type> m_node_relation;
    /**
     * @brief Contains every node of every way we have seen.
     *
     * This is used later to find all intersecting ways.
     */
    tbb::concurrent_set<osmium::object_id_type> node_ids;
    /**
     * @brief Storage for relations
     *
     * Registered relations are also stored here.
     */
    ExtractionRelationContainer &relations_stash;
};

} // namespace osrm::extractor::area

#endif // AREA_MANAGER.HPP
