#ifndef OSRM_EXTRACTOR_AREA_AREA_MANAGER_HPP
#define OSRM_EXTRACTOR_AREA_AREA_MANAGER_HPP

#include "extractor/extraction_relation.hpp"
#include "extractor/node_locations_for_ways.hpp"

#include <oneapi/tbb/concurrent_map.h>
#include <oneapi/tbb/concurrent_vector.h>
#include <oneapi/tbb/mutex.h>
#include <osmium/area/assembler_config.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/relations/relations_database.hpp>
#include <osmium/relations/relations_manager.hpp>

namespace osrm::extractor::area
{

class AreaManager
    : public osmium::relations::RelationsManager<AreaManager, false, true, false, false>
{
    osmium::area::AssemblerConfig m_assembler_config;
    extractor::NodeLocationsForWays *node_locations_for_ways;
    using MutexType = tbb::mutex;
    /** Mutex to protect relations_manager */
    MutexType mutex;

  public:
    AreaManager() { m_assembler_config.debug_level = 0; };

    void way(const osmium::Way &way);
    void relation(const osmium::Relation &relation);
    bool new_member(const osmium::Relation &,
                    const osmium::RelationMember &member,
                    std::size_t /*n*/) const noexcept;
    void prepare_for_lookup(extractor::NodeLocationsForWays &node_locations_for_ways);
    bool is_registered_closed_way(osmium::object_id_type way_id) const;
    ExtractionRelationContainer::RelationIDList get_relations(const osmium::Way &way) const;

    void complete_relation(const osmium::Relation &relation);
    void after_way(const osmium::Way &way);

    std::size_t number_of_ways{0};
    std::size_t number_of_relations{0};

    /**
     * This collects the closed ways and the outer ring members of the relations we have
     * registered for meshing. They are also used for collecting the intersecting ways.
     */
    tbb::concurrent_vector<osmium::object_id_type> registered_closed_ways;
    /** way_id -> rel_id: if way is a ring of rel */
    tbb::concurrent_map<osmium::object_id_type, osmium::object_id_type> m_ring_of;
};

} // namespace osrm::extractor::area

#endif // AREA_MANAGER.HPP
