#ifndef INTERNAL_DATAFACADE_HPP
#define INTERNAL_DATAFACADE_HPP

// implements all data storage when shared memory is _NOT_ used

#include "engine/datafacade/datafacade_base.hpp"

#include "extractor/guidance/turn_instruction.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"

#include "engine/geospatial_query.hpp"
#include "extractor/compressed_edge_container.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "storage/storage_config.hpp"
#include "util/graph_loader.hpp"
#include "util/io.hpp"
#include "util/range_table.hpp"
#include "util/rectangle.hpp"
#include "util/shared_memory_vector_wrapper.hpp"
#include "util/simple_logger.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/typedefs.hpp"

#include "osrm/coordinate.hpp"

#include <cstddef>
#include <cstdlib>

#include <algorithm>
#include <fstream>
#include <ios>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/assert.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/tss.hpp>

namespace osrm
{
namespace engine
{
namespace datafacade
{

class InternalDataFacade final : public BaseDataFacade
{

  private:
    using super = BaseDataFacade;
    using QueryGraph = util::StaticGraph<typename super::EdgeData>;
    using InputEdge = QueryGraph::InputEdge;
    using RTreeLeaf = super::RTreeLeaf;
    using InternalRTree =
        util::StaticRTree<RTreeLeaf, util::ShM<util::Coordinate, false>::vector, false>;
    using InternalGeospatialQuery = GeospatialQuery<InternalRTree, BaseDataFacade>;

    InternalDataFacade() {}

    unsigned m_check_sum;
    unsigned m_number_of_nodes;
    std::unique_ptr<QueryGraph> m_query_graph;
    std::string m_timestamp;

    util::ShM<util::Coordinate, false>::vector m_coordinate_list;
    util::ShM<NodeID, false>::vector m_via_node_list;
    util::ShM<unsigned, false>::vector m_name_ID_list;
    util::ShM<extractor::guidance::TurnInstruction, false>::vector m_turn_instruction_list;
    util::ShM<extractor::TravelMode, false>::vector m_travel_mode_list;
    util::ShM<char, false>::vector m_names_char_list;
    util::ShM<unsigned, false>::vector m_geometry_indices;
    util::ShM<extractor::CompressedEdgeContainer::CompressedEdge, false>::vector m_geometry_list;
    util::ShM<bool, false>::vector m_is_core_node;
    util::ShM<unsigned, false>::vector m_segment_weights;
    util::ShM<uint8_t, false>::vector m_datasource_list;
    util::ShM<std::string, false>::vector m_datasource_names;
    extractor::ProfileProperties m_profile_properties;

    std::unique_ptr<InternalRTree> m_static_rtree;
    std::unique_ptr<InternalGeospatialQuery> m_geospatial_query;
    boost::filesystem::path ram_index_path;
    boost::filesystem::path file_index_path;
    util::RangeTable<16, false> m_name_table;

    // bearing classes by node based node
    util::ShM<BearingClassID, false>::vector m_bearing_class_id_table;
    // entry class IDs by edge based egde
    util::ShM<EntryClassID, false>::vector m_entry_class_id_list;
    // the look-up table for entry classes. An entry class lists the possibility of entry for all
    // available turns. For every turn, there is an associated entry class.
    util::ShM<util::guidance::EntryClass, false>::vector m_entry_class_table;
    // the look-up table for distinct bearing classes. A bearing class lists the available bearings
    // at an intersection
    util::RangeTable<16, false> m_bearing_ranges_table;
    util::ShM<DiscreteBearing, false>::vector m_bearing_values_table;

    void LoadProfileProperties(const boost::filesystem::path &properties_path)
    {
        boost::filesystem::ifstream in_stream(properties_path);
        if (!in_stream)
        {
            throw util::exception("Could not open " + properties_path.string() + " for reading.");
        }

        in_stream.read(reinterpret_cast<char *>(&m_profile_properties),
                       sizeof(m_profile_properties));
    }

    void LoadTimestamp(const boost::filesystem::path &timestamp_path)
    {
        util::SimpleLogger().Write() << "Loading Timestamp";
        boost::filesystem::ifstream timestamp_stream(timestamp_path);
        if (!timestamp_stream)
        {
            throw util::exception("Could not open " + timestamp_path.string() + " for reading.");
        }
        getline(timestamp_stream, m_timestamp);
    }

    void LoadGraph(const boost::filesystem::path &hsgr_path)
    {
        util::ShM<QueryGraph::NodeArrayEntry, false>::vector node_list;
        util::ShM<QueryGraph::EdgeArrayEntry, false>::vector edge_list;

        util::SimpleLogger().Write() << "loading graph from " << hsgr_path.string();

        m_number_of_nodes = readHSGRFromStream(hsgr_path, node_list, edge_list, &m_check_sum);

        BOOST_ASSERT_MSG(0 != node_list.size(), "node list empty");
        // BOOST_ASSERT_MSG(0 != edge_list.size(), "edge list empty");
        util::SimpleLogger().Write() << "loaded " << node_list.size() << " nodes and "
                                     << edge_list.size() << " edges";
        m_query_graph = std::unique_ptr<QueryGraph>(new QueryGraph(node_list, edge_list));

        BOOST_ASSERT_MSG(0 == node_list.size(), "node list not flushed");
        BOOST_ASSERT_MSG(0 == edge_list.size(), "edge list not flushed");
        util::SimpleLogger().Write() << "Data checksum is " << m_check_sum;
    }

    void LoadNodeAndEdgeInformation(const boost::filesystem::path &nodes_file,
                                    const boost::filesystem::path &edges_file)
    {
        boost::filesystem::ifstream nodes_input_stream(nodes_file, std::ios::binary);

        extractor::QueryNode current_node;
        unsigned number_of_coordinates = 0;
        nodes_input_stream.read((char *)&number_of_coordinates, sizeof(unsigned));
        m_coordinate_list.resize(number_of_coordinates);
        for (unsigned i = 0; i < number_of_coordinates; ++i)
        {
            nodes_input_stream.read((char *)&current_node, sizeof(extractor::QueryNode));
            m_coordinate_list[i] = util::Coordinate(current_node.lon, current_node.lat);
            BOOST_ASSERT(m_coordinate_list[i].IsValid());
        }

        boost::filesystem::ifstream edges_input_stream(edges_file, std::ios::binary);
        unsigned number_of_edges = 0;
        edges_input_stream.read((char *)&number_of_edges, sizeof(unsigned));
        m_via_node_list.resize(number_of_edges);
        m_name_ID_list.resize(number_of_edges);
        m_turn_instruction_list.resize(number_of_edges);
        m_travel_mode_list.resize(number_of_edges);
        m_entry_class_id_list.resize(number_of_edges);

        extractor::OriginalEdgeData current_edge_data;
        for (unsigned i = 0; i < number_of_edges; ++i)
        {
            edges_input_stream.read((char *)&(current_edge_data),
                                    sizeof(extractor::OriginalEdgeData));
            m_via_node_list[i] = current_edge_data.via_node;
            m_name_ID_list[i] = current_edge_data.name_id;
            m_turn_instruction_list[i] = current_edge_data.turn_instruction;
            m_travel_mode_list[i] = current_edge_data.travel_mode;
            m_entry_class_id_list[i] = current_edge_data.entry_classid;
        }
    }

    void LoadCoreInformation(const boost::filesystem::path &core_data_file)
    {
        std::ifstream core_stream(core_data_file.string().c_str(), std::ios::binary);
        unsigned number_of_markers;
        core_stream.read((char *)&number_of_markers, sizeof(unsigned));

        std::vector<char> unpacked_core_markers(number_of_markers);
        core_stream.read((char *)unpacked_core_markers.data(), sizeof(char) * number_of_markers);

        // in this case we have nothing to do
        if (number_of_markers <= 0)
        {
            return;
        }

        m_is_core_node.resize(number_of_markers);
        for (auto i = 0u; i < number_of_markers; ++i)
        {
            BOOST_ASSERT(unpacked_core_markers[i] == 0 || unpacked_core_markers[i] == 1);
            m_is_core_node[i] = unpacked_core_markers[i] == 1;
        }
    }

    void LoadGeometries(const boost::filesystem::path &geometry_file)
    {
        std::ifstream geometry_stream(geometry_file.string().c_str(), std::ios::binary);
        unsigned number_of_indices = 0;
        unsigned number_of_compressed_geometries = 0;

        geometry_stream.read((char *)&number_of_indices, sizeof(unsigned));

        m_geometry_indices.resize(number_of_indices);
        if (number_of_indices > 0)
        {
            geometry_stream.read((char *)&(m_geometry_indices[0]),
                                 number_of_indices * sizeof(unsigned));
        }

        geometry_stream.read((char *)&number_of_compressed_geometries, sizeof(unsigned));

        BOOST_ASSERT(m_geometry_indices.back() == number_of_compressed_geometries);
        m_geometry_list.resize(number_of_compressed_geometries);

        if (number_of_compressed_geometries > 0)
        {
            geometry_stream.read((char *)&(m_geometry_list[0]),
                                 number_of_compressed_geometries *
                                     sizeof(extractor::CompressedEdgeContainer::CompressedEdge));
        }
    }

    void LoadDatasourceInfo(const boost::filesystem::path &datasource_names_file,
                            const boost::filesystem::path &datasource_indexes_file)
    {
        boost::filesystem::ifstream datasources_stream(datasource_indexes_file, std::ios::binary);
        if (!datasources_stream)
        {
            throw util::exception("Could not open " + datasource_indexes_file.string() +
                                  " for reading!");
        }
        BOOST_ASSERT(datasources_stream);

        std::size_t number_of_datasources = 0;
        datasources_stream.read(reinterpret_cast<char *>(&number_of_datasources),
                                sizeof(std::size_t));
        if (number_of_datasources > 0)
        {
            m_datasource_list.resize(number_of_datasources);
            datasources_stream.read(reinterpret_cast<char *>(&(m_datasource_list[0])),
                                    number_of_datasources * sizeof(uint8_t));
        }

        boost::filesystem::ifstream datasourcenames_stream(datasource_names_file, std::ios::binary);
        if (!datasourcenames_stream)
        {
            throw util::exception("Could not open " + datasource_names_file.string() +
                                  " for reading!");
        }
        BOOST_ASSERT(datasourcenames_stream);
        std::string name;
        while (std::getline(datasourcenames_stream, name))
        {
            m_datasource_names.push_back(std::move(name));
        }
    }

    void LoadRTree()
    {
        BOOST_ASSERT_MSG(!m_coordinate_list.empty(), "coordinates must be loaded before r-tree");

        m_static_rtree.reset(new InternalRTree(ram_index_path, file_index_path, m_coordinate_list));
        m_geospatial_query.reset(
            new InternalGeospatialQuery(*m_static_rtree, m_coordinate_list, *this));
    }

    void LoadStreetNames(const boost::filesystem::path &names_file)
    {
        boost::filesystem::ifstream name_stream(names_file, std::ios::binary);

        name_stream >> m_name_table;

        unsigned number_of_chars = 0;
        name_stream.read((char *)&number_of_chars, sizeof(unsigned));
        BOOST_ASSERT_MSG(0 != number_of_chars, "name file broken");
        m_names_char_list.resize(number_of_chars + 1); //+1 gives sentinel element
        name_stream.read((char *)&m_names_char_list[0], number_of_chars * sizeof(char));
        if (0 == m_names_char_list.size())
        {
            util::SimpleLogger().Write(logWARNING) << "list of street names is empty";
        }
    }

    void LoadIntersectionClasses(const boost::filesystem::path &intersection_class_file)
    {
        std::ifstream intersection_stream(intersection_class_file.string(), std::ios::binary);
        if (!intersection_stream)
            throw util::exception("Could not open " + intersection_class_file.string() +
                                  " for reading.");

        if (!util::readAndCheckFingerprint(intersection_stream))
            throw util::exception("Fingeprint does not match in " +
                                  intersection_class_file.string());

        {
            util::SimpleLogger().Write(logINFO) << "Loading Bearing Class IDs";
            std::vector<BearingClassID> bearing_class_id;
            if (!util::deserializeVector(intersection_stream, bearing_class_id))
                throw util::exception("Reading from " + intersection_class_file.string() + " failed.");

            m_bearing_class_id_table.resize(bearing_class_id.size());
            std::copy(bearing_class_id.begin(), bearing_class_id.end(),
                      &m_bearing_class_id_table[0]);
        }
        {
            util::SimpleLogger().Write(logINFO) << "Loading Bearing Classes";
            // read the range table
            intersection_stream >> m_bearing_ranges_table;
            std::vector<util::guidance::BearingClass> bearing_classes;
            // and the actual bearing values
            std::uint64_t num_bearings;
            intersection_stream >> num_bearings;
            m_bearing_values_table.resize(num_bearings);
            intersection_stream.read(reinterpret_cast<char *>(&m_bearing_values_table[0]),
                                     sizeof(m_bearing_values_table[0]) * num_bearings);
            if (!static_cast<bool>(intersection_stream))
                throw util::exception("Reading from " + intersection_class_file.string() + " failed.");
        }
        {
            util::SimpleLogger().Write(logINFO) << "Loading Entry Classes";
            std::vector<util::guidance::EntryClass> entry_classes;
            if (!util::deserializeVector(intersection_stream, entry_classes))
                throw util::exception("Reading from " + intersection_class_file.string() + " failed.");

            m_entry_class_table.resize(entry_classes.size());
            std::copy(entry_classes.begin(), entry_classes.end(), &m_entry_class_table[0]);
        }
    }

  public:
    virtual ~InternalDataFacade()
    {
        m_static_rtree.reset();
        m_geospatial_query.reset();
    }

    explicit InternalDataFacade(const storage::StorageConfig &config)
    {
        ram_index_path = config.ram_index_path;
        file_index_path = config.file_index_path;

        util::SimpleLogger().Write() << "loading graph data";
        LoadGraph(config.hsgr_data_path);

        util::SimpleLogger().Write() << "loading edge information";
        LoadNodeAndEdgeInformation(config.nodes_data_path, config.edges_data_path);

        util::SimpleLogger().Write() << "loading core information";
        LoadCoreInformation(config.core_data_path);

        util::SimpleLogger().Write() << "loading geometries";
        LoadGeometries(config.geometries_path);

        util::SimpleLogger().Write() << "loading datasource info";
        LoadDatasourceInfo(config.datasource_names_path, config.datasource_indexes_path);

        util::SimpleLogger().Write() << "loading timestamp";
        LoadTimestamp(config.timestamp_path);

        util::SimpleLogger().Write() << "loading profile properties";
        LoadProfileProperties(config.properties_path);

        util::SimpleLogger().Write() << "loading street names";
        LoadStreetNames(config.names_data_path);

        util::SimpleLogger().Write() << "loading rtree";
        LoadRTree();

        util::SimpleLogger().Write() << "loading intersection class data";
        LoadIntersectionClasses(config.intersection_class_path);
    }

    // search graph access
    unsigned GetNumberOfNodes() const override final { return m_query_graph->GetNumberOfNodes(); }

    unsigned GetNumberOfEdges() const override final { return m_query_graph->GetNumberOfEdges(); }

    unsigned GetOutDegree(const NodeID n) const override final
    {
        return m_query_graph->GetOutDegree(n);
    }

    NodeID GetTarget(const EdgeID e) const override final { return m_query_graph->GetTarget(e); }

    EdgeData &GetEdgeData(const EdgeID e) const override final
    {
        return m_query_graph->GetEdgeData(e);
    }

    EdgeID BeginEdges(const NodeID n) const override final { return m_query_graph->BeginEdges(n); }

    EdgeID EndEdges(const NodeID n) const override final { return m_query_graph->EndEdges(n); }

    EdgeRange GetAdjacentEdgeRange(const NodeID node) const override final
    {
        return m_query_graph->GetAdjacentEdgeRange(node);
    }

    // searches for a specific edge
    EdgeID FindEdge(const NodeID from, const NodeID to) const override final
    {
        return m_query_graph->FindEdge(from, to);
    }

    EdgeID FindEdgeInEitherDirection(const NodeID from, const NodeID to) const override final
    {
        return m_query_graph->FindEdgeInEitherDirection(from, to);
    }

    EdgeID
    FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const override final
    {
        return m_query_graph->FindEdgeIndicateIfReverse(from, to, result);
    }

    // node and edge information access
    util::Coordinate GetCoordinateOfNode(const unsigned id) const override final
    {
        return m_coordinate_list[id];
    }

    extractor::guidance::TurnInstruction
    GetTurnInstructionForEdgeID(const unsigned id) const override final
    {
        return m_turn_instruction_list.at(id);
    }

    extractor::TravelMode GetTravelModeForEdgeID(const unsigned id) const override final
    {
        return m_travel_mode_list.at(id);
    }

    std::vector<RTreeLeaf> GetEdgesInBox(const util::Coordinate south_west,
                                         const util::Coordinate north_east) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());
        const util::RectangleInt2D bbox{south_west.lon, north_east.lon, south_west.lat,
                                        north_east.lat};
        return m_geospatial_query->Search(bbox);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate input_coordinate,
                               const float max_distance) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodesInRange(input_coordinate, max_distance);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate input_coordinate,
                               const float max_distance,
                               const int bearing,
                               const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodesInRange(input_coordinate, max_distance,
                                                              bearing, bearing_range);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(input_coordinate, max_results);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const double max_distance) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(input_coordinate, max_results, max_distance);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const int bearing,
                        const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(input_coordinate, max_results, bearing,
                                                       bearing_range);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const double max_distance,
                        const int bearing,
                        const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(input_coordinate, max_results, max_distance,
                                                       bearing, bearing_range);
    }

    std::pair<PhantomNode, PhantomNode> NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::Coordinate input_coordinate, const double max_distance) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate, max_distance);
    }

    std::pair<PhantomNode, PhantomNode> NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::Coordinate input_coordinate) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate);
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const double max_distance,
                                                      const int bearing,
                                                      const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate, max_distance, bearing, bearing_range);
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const int bearing,
                                                      const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate, bearing, bearing_range);
    }

    unsigned GetCheckSum() const override final { return m_check_sum; }

    unsigned GetNameIndexFromEdgeID(const unsigned id) const override final
    {
        return m_name_ID_list.at(id);
    }

    std::string GetNameForID(const unsigned name_id) const override final
    {
        if (std::numeric_limits<unsigned>::max() == name_id)
        {
            return "";
        }
        auto range = m_name_table.GetRange(name_id);

        std::string result;
        result.reserve(range.size());
        if (range.begin() != range.end())
        {
            result.resize(range.back() - range.front() + 1);
            std::copy(m_names_char_list.begin() + range.front(),
                      m_names_char_list.begin() + range.back() + 1, result.begin());
        }
        return result;
    }

    virtual unsigned GetGeometryIndexForEdgeID(const unsigned id) const override final
    {
        return m_via_node_list.at(id);
    }

    virtual std::size_t GetCoreSize() const override final { return m_is_core_node.size(); }

    virtual bool IsCoreNode(const NodeID id) const override final
    {
        if (m_is_core_node.size() > 0)
        {
            return m_is_core_node[id];
        }
        else
        {
            return false;
        }
    }

    virtual void GetUncompressedGeometry(const EdgeID id,
                                         std::vector<NodeID> &result_nodes) const override final
    {
        const unsigned begin = m_geometry_indices.at(id);
        const unsigned end = m_geometry_indices.at(id + 1);

        result_nodes.clear();
        result_nodes.reserve(end - begin);
        std::for_each(m_geometry_list.begin() + begin, m_geometry_list.begin() + end,
                      [&](const osrm::extractor::CompressedEdgeContainer::CompressedEdge &edge) {
                          result_nodes.emplace_back(edge.node_id);
                      });
    }

    virtual void
    GetUncompressedWeights(const EdgeID id,
                           std::vector<EdgeWeight> &result_weights) const override final
    {
        const unsigned begin = m_geometry_indices.at(id);
        const unsigned end = m_geometry_indices.at(id + 1);

        result_weights.clear();
        result_weights.reserve(end - begin);
        std::for_each(m_geometry_list.begin() + begin, m_geometry_list.begin() + end,
                      [&](const osrm::extractor::CompressedEdgeContainer::CompressedEdge &edge) {
                          result_weights.emplace_back(edge.weight);
                      });
    }

    // Returns the data source ids that were used to supply the edge
    // weights.
    virtual void
    GetUncompressedDatasources(const EdgeID id,
                               std::vector<uint8_t> &result_datasources) const override final
    {
        const unsigned begin = m_geometry_indices.at(id);
        const unsigned end = m_geometry_indices.at(id + 1);

        result_datasources.clear();
        result_datasources.reserve(end - begin);

        // If there was no datasource info, return an array of 0's.
        if (m_datasource_list.empty())
        {
            for (unsigned i = 0; i < end - begin; ++i)
            {
                result_datasources.push_back(0);
            }
        }
        else
        {
            std::for_each(
                m_datasource_list.begin() + begin, m_datasource_list.begin() + end,
                [&](const uint8_t &datasource_id) { result_datasources.push_back(datasource_id); });
        }
    }

    virtual std::string GetDatasourceName(const uint8_t datasource_name_id) const override final
    {
        BOOST_ASSERT(m_datasource_names.size() >= 1);
        BOOST_ASSERT(m_datasource_names.size() > datasource_name_id);
        return m_datasource_names[datasource_name_id];
    }

    std::string GetTimestamp() const override final { return m_timestamp; }

    bool GetContinueStraightDefault() const override final
    {
        return m_profile_properties.continue_straight_at_waypoint;
    }

    BearingClassID GetBearingClassID(const NodeID nid) const override final
    {
        return m_bearing_class_id_table.at(nid);
    }

    util::guidance::BearingClass
    GetBearingClass(const BearingClassID bearing_class_id) const override final
    {
        BOOST_ASSERT(bearing_class_id != INVALID_BEARING_CLASSID);
        auto range = m_bearing_ranges_table.GetRange(bearing_class_id);

        util::guidance::BearingClass result;

        for (auto itr = m_bearing_values_table.begin() + range.front();
             itr != m_bearing_values_table.begin() + range.back() + 1; ++itr)
            result.add(*itr);

        return result;
    }

    EntryClassID GetEntryClassID(const EdgeID eid) const override final
    {
        return m_entry_class_id_list.at(eid);
    }

    util::guidance::EntryClass GetEntryClass(const EntryClassID entry_class_id) const override final
    {
        return m_entry_class_table.at(entry_class_id);
    }
};
}
}
}

#endif // INTERNAL_DATAFACADE_HPP
