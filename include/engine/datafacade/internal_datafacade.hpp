#ifndef INTERNAL_DATAFACADE_HPP
#define INTERNAL_DATAFACADE_HPP

// implements all data storage when shared memory is _NOT_ used

#include "engine/datafacade/datafacade_base.hpp"

#include "extractor/guidance/turn_instruction.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "storage/io.hpp"
#include "storage/storage_config.hpp"
#include "engine/geospatial_query.hpp"
#include "util/graph_loader.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/io.hpp"
#include "util/packed_vector.hpp"
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
    std::unique_ptr<QueryGraph> m_query_graph;
    std::string m_timestamp;

    util::ShM<util::Coordinate, false>::vector m_coordinate_list;
    util::PackedVector<OSMNodeID, false> m_osmnodeid_list;
    util::ShM<GeometryID, false>::vector m_via_geometry_list;
    util::ShM<unsigned, false>::vector m_name_ID_list;
    util::ShM<extractor::guidance::TurnInstruction, false>::vector m_turn_instruction_list;
    util::ShM<LaneDataID, false>::vector m_lane_data_id;
    util::ShM<util::guidance::LaneTupleIdPair, false>::vector m_lane_tuple_id_pairs;
    util::ShM<extractor::TravelMode, false>::vector m_travel_mode_list;
    util::ShM<char, false>::vector m_names_char_list;
    util::ShM<unsigned, false>::vector m_geometry_indices;
    util::ShM<NodeID, false>::vector m_geometry_node_list;
    util::ShM<EdgeWeight, false>::vector m_geometry_fwd_weight_list;
    util::ShM<EdgeWeight, false>::vector m_geometry_rev_weight_list;
    util::ShM<bool, false>::vector m_is_core_node;
    util::ShM<unsigned, false>::vector m_segment_weights;
    util::ShM<uint8_t, false>::vector m_datasource_list;
    util::ShM<std::string, false>::vector m_datasource_names;
    util::ShM<std::uint32_t, false>::vector m_lane_description_offsets;
    util::ShM<extractor::guidance::TurnLaneType::Mask, false>::vector m_lane_description_masks;
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
    // bearings pre/post turn
    util::ShM<util::guidance::TurnBearing, false>::vector m_pre_turn_bearing;
    util::ShM<util::guidance::TurnBearing, false>::vector m_post_turn_bearing;
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

    void LoadLaneTupleIdPairs(const boost::filesystem::path &lane_data_path)
    {
        boost::filesystem::ifstream in_stream(lane_data_path);
        if (!in_stream)
        {
            throw util::exception("Could not open " + lane_data_path.string() + " for reading.");
        }
        std::uint64_t size;
        in_stream.read(reinterpret_cast<char *>(&size), sizeof(size));
        m_lane_tuple_id_pairs.resize(size);
        in_stream.read(reinterpret_cast<char *>(&m_lane_tuple_id_pairs[0]),
                       sizeof(m_lane_tuple_id_pairs) * size);
    }

    void LoadTimestamp(const boost::filesystem::path &timestamp_path)
    {
        util::SimpleLogger().Write() << "Loading Timestamp";

        boost::filesystem::ifstream timestamp_stream(timestamp_path);
        if (!timestamp_stream)
        {
            throw util::exception("Could not open " + timestamp_path.string() + " for reading.");
        }

        auto timestamp_size = storage::io::readTimestampSize(timestamp_stream);
        m_timestamp.resize(timestamp_size);
        storage::io::readTimestamp(timestamp_stream, &m_timestamp.front(), timestamp_size);
    }

    void LoadGraph(const boost::filesystem::path &hsgr_path)
    {
        boost::filesystem::ifstream hsgr_input_stream(hsgr_path);
        if (!hsgr_input_stream)
        {
            throw util::exception("Could not open " + hsgr_path.string() + " for reading.");
        }

        auto header = storage::io::readHSGRHeader(hsgr_input_stream);
        m_check_sum = header.checksum;

        util::ShM<QueryGraph::NodeArrayEntry, false>::vector node_list(header.number_of_nodes);
        util::ShM<QueryGraph::EdgeArrayEntry, false>::vector edge_list(header.number_of_edges);

        storage::io::readHSGR(hsgr_input_stream,
                              node_list.data(),
                              header.number_of_nodes,
                              edge_list.data(),
                              header.number_of_edges);

        m_query_graph = std::unique_ptr<QueryGraph>(new QueryGraph(node_list, edge_list));

        util::SimpleLogger().Write() << "Data checksum is " << m_check_sum;
    }

    void LoadNodeAndEdgeInformation(const boost::filesystem::path &nodes_file_path,
                                    const boost::filesystem::path &edges_file_path)
    {
        boost::filesystem::ifstream nodes_input_stream(nodes_file_path, std::ios::binary);
        if (!nodes_input_stream)
        {
            throw util::exception("Could not open " + nodes_file_path.string() + " for reading.");
        }

        std::uint32_t number_of_coordinates = storage::io::readNodesSize(nodes_input_stream);
        m_coordinate_list.resize(number_of_coordinates);
        m_osmnodeid_list.reserve(number_of_coordinates);
        storage::io::readNodesData(
            nodes_input_stream, m_coordinate_list.data(), m_osmnodeid_list, number_of_coordinates);

        boost::filesystem::ifstream edges_input_stream(edges_file_path, std::ios::binary);
        if (!edges_input_stream)
        {
            throw util::exception("Could not open " + edges_file_path.string() + " for reading.");
        }
        auto number_of_edges = storage::io::readEdgesSize(edges_input_stream);
        m_via_geometry_list.resize(number_of_edges);
        m_name_ID_list.resize(number_of_edges);
        m_turn_instruction_list.resize(number_of_edges);
        m_lane_data_id.resize(number_of_edges);
        m_travel_mode_list.resize(number_of_edges);
        m_entry_class_id_list.resize(number_of_edges);
        m_pre_turn_bearing.resize(number_of_edges);
        m_post_turn_bearing.resize(number_of_edges);

        storage::io::readEdgesData(edges_input_stream,
                                   m_via_geometry_list.data(),
                                   m_name_ID_list.data(),
                                   m_turn_instruction_list.data(),
                                   m_lane_data_id.data(),
                                   m_travel_mode_list.data(),
                                   m_entry_class_id_list.data(),
                                   m_pre_turn_bearing.data(),
                                   m_post_turn_bearing.data(),
                                   number_of_edges);
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
        m_geometry_node_list.resize(number_of_compressed_geometries);
        m_geometry_fwd_weight_list.resize(number_of_compressed_geometries);
        m_geometry_rev_weight_list.resize(number_of_compressed_geometries);

        if (number_of_compressed_geometries > 0)
        {
            geometry_stream.read((char *)&(m_geometry_node_list[0]),
                                 number_of_compressed_geometries * sizeof(NodeID));

            geometry_stream.read((char *)&(m_geometry_fwd_weight_list[0]),
                                 number_of_compressed_geometries * sizeof(EdgeWeight));

            geometry_stream.read((char *)&(m_geometry_rev_weight_list[0]),
                                 number_of_compressed_geometries * sizeof(EdgeWeight));
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

        auto number_of_datasources = storage::io::readDatasourceIndexesSize(datasources_stream);
        if (number_of_datasources > 0)
        {
            m_datasource_list.resize(number_of_datasources);
            storage::io::readDatasourceIndexes(
                datasources_stream, &m_datasource_list.front(), number_of_datasources);
        }

        boost::filesystem::ifstream datasourcenames_stream(datasource_names_file, std::ios::binary);
        if (!datasourcenames_stream)
        {
            throw util::exception("Could not open " + datasource_names_file.string() +
                                  " for reading!");
        }
        BOOST_ASSERT(datasourcenames_stream);

        auto datasource_names_data = storage::io::readDatasourceNamesData(datasourcenames_stream);
        m_datasource_names.resize(datasource_names_data.lengths.size());
        for (std::uint32_t i = 0; i < datasource_names_data.lengths.size(); ++i)
        {
            auto name_begin =
                datasource_names_data.names.begin() + datasource_names_data.offsets[i];
            auto name_end = datasource_names_data.names.begin() + datasource_names_data.offsets[i] +
                            datasource_names_data.lengths[i];
            m_datasource_names[i] = std::move(std::string(name_begin, name_end));
        }
    }

    void LoadRTree()
    {
        BOOST_ASSERT_MSG(!m_coordinate_list.empty(), "coordinates must be loaded before r-tree");

        m_static_rtree.reset(new InternalRTree(ram_index_path, file_index_path, m_coordinate_list));
        m_geospatial_query.reset(
            new InternalGeospatialQuery(*m_static_rtree, m_coordinate_list, *this));
    }

    void LoadLaneDescriptions(const boost::filesystem::path &lane_description_file)
    {
        if (!util::deserializeAdjacencyArray(lane_description_file.string(),
                                             m_lane_description_offsets,
                                             m_lane_description_masks))
            util::SimpleLogger().Write(logWARNING) << "Failed to read turn lane descriptions from "
                                                   << lane_description_file.string();
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
                throw util::exception("Reading from " + intersection_class_file.string() +
                                      " failed.");

            m_bearing_class_id_table.resize(bearing_class_id.size());
            std::copy(
                bearing_class_id.begin(), bearing_class_id.end(), &m_bearing_class_id_table[0]);
        }
        {
            util::SimpleLogger().Write(logINFO) << "Loading Bearing Classes";
            // read the range table
            intersection_stream >> m_bearing_ranges_table;
            std::vector<util::guidance::BearingClass> bearing_classes;
            // and the actual bearing values
            std::uint64_t num_bearings;
            intersection_stream.read(reinterpret_cast<char *>(&num_bearings), sizeof(num_bearings));
            m_bearing_values_table.resize(num_bearings);
            intersection_stream.read(reinterpret_cast<char *>(&m_bearing_values_table[0]),
                                     sizeof(m_bearing_values_table[0]) * num_bearings);
            if (!static_cast<bool>(intersection_stream))
                throw util::exception("Reading from " + intersection_class_file.string() +
                                      " failed.");
        }
        {
            util::SimpleLogger().Write(logINFO) << "Loading Entry Classes";
            std::vector<util::guidance::EntryClass> entry_classes;
            if (!util::deserializeVector(intersection_stream, entry_classes))
                throw util::exception("Reading from " + intersection_class_file.string() +
                                      " failed.");

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

        util::SimpleLogger().Write() << "loading lane tags";
        LoadLaneDescriptions(config.turn_lane_description_path);

        util::SimpleLogger().Write() << "loading rtree";
        LoadRTree();

        util::SimpleLogger().Write() << "loading intersection class data";
        LoadIntersectionClasses(config.intersection_class_path);

        util::SimpleLogger().Write() << "Loading Lane Data Pairs";
        LoadLaneTupleIdPairs(config.turn_lane_data_path);
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

    EdgeID FindSmallestEdge(const NodeID from,
                            const NodeID to,
                            std::function<bool(EdgeData)> filter) const override final
    {
        return m_query_graph->FindSmallestEdge(from, to, filter);
    }

    // node and edge information access
    util::Coordinate GetCoordinateOfNode(const unsigned id) const override final
    {
        return m_coordinate_list[id];
    }

    OSMNodeID GetOSMNodeIDOfNode(const unsigned id) const override final
    {
        return m_osmnodeid_list.at(id);
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
        const util::RectangleInt2D bbox{
            south_west.lon, north_east.lon, south_west.lat, north_east.lat};
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

        return m_geospatial_query->NearestPhantomNodesInRange(
            input_coordinate, max_distance, bearing, bearing_range);
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

        return m_geospatial_query->NearestPhantomNodes(
            input_coordinate, max_results, bearing, bearing_range);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const double max_distance,
                        const int bearing,
                        const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(
            input_coordinate, max_results, max_distance, bearing, bearing_range);
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
                      m_names_char_list.begin() + range.back() + 1,
                      result.begin());
        }
        return result;
    }

    std::string GetRefForID(const unsigned name_id) const override final
    {
        // We store the ref after the name, destination and pronunciation of a street.
        // We do this to get around the street length limit of 255 which would hit
        // if we concatenate these. Order (see extractor_callbacks):
        // name (0), destination (1), pronunciation (2), ref (3)
        return GetNameForID(name_id + 3);
    }

    std::string GetPronunciationForID(const unsigned name_id) const override final
    {
        // We store the pronunciation after the name and destination of a street.
        // We do this to get around the street length limit of 255 which would hit
        // if we concatenate these. Order (see extractor_callbacks):
        // name (0), destination (1), pronunciation (2)
        return GetNameForID(name_id + 2);
    }

    std::string GetDestinationsForID(const unsigned name_id) const override final
    {
        // We store the destination after the name of a street.
        // We do this to get around the street length limit of 255 which would hit
        // if we concatenate these. Order (see extractor_callbacks):
        // name (0), destination (1), pronunciation (2)
        return GetNameForID(name_id + 1);
    }

    virtual GeometryID GetGeometryIndexForEdgeID(const unsigned id) const override final
    {
        return m_via_geometry_list.at(id);
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

    virtual std::vector<NodeID> GetUncompressedForwardGeometry(const EdgeID id) const override final
    {
        /*
         * NodeID's for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_node_list vector. For
         * forward geometries of bi-directional edges, edges 2 to
         * n of that edge need to be read.
         */
        const unsigned begin = m_geometry_indices.at(id);
        const unsigned end = m_geometry_indices.at(id + 1);

        std::vector<NodeID> result_nodes;

        result_nodes.resize(end - begin);

        std::copy(m_geometry_node_list.begin() + begin,
                  m_geometry_node_list.begin() + end,
                  result_nodes.begin());

        return result_nodes;
    }

    virtual std::vector<NodeID> GetUncompressedReverseGeometry(const EdgeID id) const override final
    {
        /*
         * NodeID's for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_node_list vector.
         * */
        const unsigned begin = m_geometry_indices.at(id);
        const unsigned end = m_geometry_indices.at(id + 1);

        std::vector<NodeID> result_nodes;

        result_nodes.resize(end - begin);

        std::copy(m_geometry_node_list.rbegin() + (m_geometry_node_list.size() - end),
                  m_geometry_node_list.rbegin() + (m_geometry_node_list.size() - begin),
                  result_nodes.begin());

        return result_nodes;
    }

    virtual std::vector<EdgeWeight>
    GetUncompressedForwardWeights(const EdgeID id) const override final
    {
        /*
         * EdgeWeights's for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_fwd_weight_list vector.
         * */
        const unsigned begin = m_geometry_indices.at(id) + 1;
        const unsigned end = m_geometry_indices.at(id + 1);

        std::vector<EdgeWeight> result_weights;
        result_weights.resize(end - begin);

        std::copy(m_geometry_fwd_weight_list.begin() + begin,
                  m_geometry_fwd_weight_list.begin() + end,
                  result_weights.begin());

        return result_weights;
    }

    virtual std::vector<EdgeWeight>
    GetUncompressedReverseWeights(const EdgeID id) const override final
    {
        /*
         * EdgeWeights for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_rev_weight_list vector. For
         * reverse weights of bi-directional edges, edges 1 to
         * n-1 of that edge need to be read in reverse.
         */
        const unsigned begin = m_geometry_indices.at(id);
        const unsigned end = m_geometry_indices.at(id + 1) - 1;

        std::vector<EdgeWeight> result_weights;
        result_weights.resize(end - begin);

        std::copy(m_geometry_rev_weight_list.rbegin() + (m_geometry_rev_weight_list.size() - end),
                  m_geometry_rev_weight_list.rbegin() + (m_geometry_rev_weight_list.size() - begin),
                  result_weights.begin());

        return result_weights;
    }

    // Returns the data source ids that were used to supply the edge
    // weights.
    virtual std::vector<uint8_t>
    GetUncompressedForwardDatasources(const EdgeID id) const override final
    {
        /*
         * Data sources for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_list vector. For
         * forward datasources of bi-directional edges, edges 2 to
         * n of that edge need to be read.
         */
        const unsigned begin = m_geometry_indices.at(id) + 1;
        const unsigned end = m_geometry_indices.at(id + 1);

        std::vector<uint8_t> result_datasources;
        result_datasources.resize(end - begin);

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
            std::copy(m_datasource_list.begin() + begin,
                      m_datasource_list.begin() + end,
                      result_datasources.begin());
        }

        return result_datasources;
    }

    // Returns the data source ids that were used to supply the edge
    // weights.
    virtual std::vector<uint8_t>
    GetUncompressedReverseDatasources(const EdgeID id) const override final
    {
        /*
         * Datasources for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_list vector. For
         * reverse datasources of bi-directional edges, edges 1 to
         * n-1 of that edge need to be read in reverse.
         */
        const unsigned begin = m_geometry_indices.at(id);
        const unsigned end = m_geometry_indices.at(id + 1) - 1;

        std::vector<uint8_t> result_datasources;
        result_datasources.resize(end - begin);

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
            std::copy(m_datasource_list.rbegin() + (m_datasource_list.size() - end),
                      m_datasource_list.rbegin() + (m_datasource_list.size() - begin),
                      result_datasources.begin());
        }

        return result_datasources;
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
             itr != m_bearing_values_table.begin() + range.back() + 1;
             ++itr)
            result.add(*itr);

        return result;
    }

    EntryClassID GetEntryClassID(const EdgeID eid) const override final
    {
        return m_entry_class_id_list.at(eid);
    }

    util::guidance::TurnBearing PreTurnBearing(const EdgeID eid) const override final
    {
        return m_pre_turn_bearing.at(eid);
    }
    util::guidance::TurnBearing PostTurnBearing(const EdgeID eid) const override final
    {
        return m_post_turn_bearing.at(eid);
    }

    util::guidance::EntryClass GetEntryClass(const EntryClassID entry_class_id) const override final
    {
        return m_entry_class_table.at(entry_class_id);
    }

    bool hasLaneData(const EdgeID id) const override final
    {
        return m_lane_data_id[id] != INVALID_LANE_DATAID;
    }

    util::guidance::LaneTupleIdPair GetLaneData(const EdgeID id) const override final
    {
        BOOST_ASSERT(hasLaneData(id));
        return m_lane_tuple_id_pairs[m_lane_data_id[id]];
    }

    extractor::guidance::TurnLaneDescription
    GetTurnDescription(const LaneDescriptionID lane_description_id) const override final
    {
        if (lane_description_id == INVALID_LANE_DESCRIPTIONID)
            return {};
        else
            return extractor::guidance::TurnLaneDescription(
                m_lane_description_masks.begin() + m_lane_description_offsets[lane_description_id],
                m_lane_description_masks.begin() +
                    m_lane_description_offsets[lane_description_id + 1]);
    }
};
}
}
}

#endif // INTERNAL_DATAFACADE_HPP
