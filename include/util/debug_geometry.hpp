#ifndef DEBUG_GEOMETRY_H
#define DEBUG_GEOMETRY_H

#include "contractor/contractor_config.hpp"
#include "extractor/query_node.hpp"
#include "osrm/coordinate.hpp"
#include <boost/filesystem/fstream.hpp>

#ifndef DEBUG_GEOMETRY

namespace osrm
{
namespace util
{

inline void DEBUG_GEOMETRY_START(const contractor::ContractorConfig & /* config */) {}
inline void DEBUG_GEOMETRY_EDGE(int /* new_segment_weight */,
                                double /* segment_length */,
                                OSMNodeID /* previous_osm_node_id */,
                                OSMNodeID /* this_osm_node_id */)
{
}
inline void DEBUG_GEOMETRY_STOP() {}

inline void DEBUG_TURNS_START(const std::string & /* debug_turns_filename */) {}
inline void DEBUG_TURN(const NodeID /* node */,
                       const std::vector<extractor::QueryNode> & /* m_node_info_list */,
                       const FixedPointCoordinate /* first_coordinate */,
                       const int /* turn_angle */,
                       const int /* turn_penalty */)
{
}
inline void DEBUG_UTURN(const NodeID /* node */,
                        const std::vector<extractor::QueryNode> & /* m_node_info_list */,
                        const int /* uturn_penalty */)
{
}
inline void DEBUG_SIGNAL(const NodeID /* node */,
                         const std::vector<extractor::QueryNode> & /* m_node_info_list */,
                         const int /* signal_penalty */)
{
}

inline void DEBUG_TURNS_STOP() {}
}
}

#else // DEBUG_GEOMETRY

#include <boost/filesystem.hpp>
#include <ctime>
#include <string>
#include <iomanip>
#include <iostream>

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

namespace osrm
{
namespace util
{

boost::filesystem::ofstream debug_geometry_file;
bool dg_output_debug_geometry = false;
bool dg_first_debug_geometry = true;
char dg_time_buffer[80];

boost::filesystem::ofstream dg_debug_turns_file;
bool dg_output_turn_debug = false;
bool dg_first_turn_debug = true;

std::unordered_map<OSMNodeID, util::FixedPointCoordinate> node_lookup_map;

inline void DEBUG_GEOMETRY_START(const contractor::ContractorConfig &config)
{
    time_t raw_time;
    struct tm *timeinfo;
    time(&raw_time);
    timeinfo = localtime(&raw_time);
    strftime(dg_time_buffer, 80, "%Y-%m-%d %H:%M %Z", timeinfo);

    boost::filesystem::ifstream nodes_input_stream{config.node_based_graph_path,
                                                   std::ios_base::in | std::ios_base::binary};

    extractor::QueryNode current_node;
    unsigned number_of_coordinates = 0;
    nodes_input_stream.read((char *)&number_of_coordinates, sizeof(unsigned));

    for (unsigned i = 0; i < number_of_coordinates; ++i)
    {
        nodes_input_stream.read((char *)&current_node, sizeof(extractor::QueryNode));
        node_lookup_map[current_node.node_id] =
            util::FixedPointCoordinate(current_node.lat, current_node.lon);
    }
    nodes_input_stream.close();

    dg_output_debug_geometry = config.debug_geometry_path != "";

    if (dg_output_debug_geometry)
    {
        debug_geometry_file.open(config.debug_geometry_path, std::ios::binary);
        debug_geometry_file << "{\"type\":\"FeatureCollection\", \"features\":[" << std::endl;
        debug_geometry_file << std::setprecision(10);
    }
}

inline void DEBUG_GEOMETRY_EDGE(int new_segment_weight,
                                double segment_length,
                                OSMNodeID previous_osm_node_id,
                                OSMNodeID this_osm_node_id)
{
    if (dg_output_debug_geometry)
    {
        if (!dg_first_debug_geometry)
            debug_geometry_file << "," << std::endl;
        debug_geometry_file << "{ \"type\":\"Feature\",\"properties\":{\"original\":false, "
                               "\"weight\":"
                            << new_segment_weight / 10.0 << ",\"speed\":"
                            << static_cast<int>(
                                   std::floor((segment_length / new_segment_weight) * 10. * 3.6))
                            << ",";
        debug_geometry_file << "\"from_node\": " << previous_osm_node_id
                            << ", \"to_node\": " << this_osm_node_id << ",";
        debug_geometry_file << "\"timestamp\": \"" << dg_time_buffer << "\"},";
        debug_geometry_file
            << "\"geometry\":{\"type\":\"LineString\",\"coordinates\":[["
            << node_lookup_map[previous_osm_node_id].lon / osrm::COORDINATE_PRECISION << ","
            << node_lookup_map[previous_osm_node_id].lat / osrm::COORDINATE_PRECISION << "],["
            << node_lookup_map[this_osm_node_id].lon / osrm::COORDINATE_PRECISION << ","
            << node_lookup_map[this_osm_node_id].lat / osrm::COORDINATE_PRECISION << "]]}}"
            << std::endl;
        dg_first_debug_geometry = false;
    }
}

inline void DEBUG_GEOMETRY_STOP()
{
    if (dg_output_debug_geometry)
    {
        debug_geometry_file << "\n]}" << std::endl;
        debug_geometry_file.close();
    }
}

inline void DEBUG_TURNS_START(const std::string &debug_turns_path)
{
    dg_output_turn_debug = debug_turns_path != "";
    if (dg_output_turn_debug)
    {
        dg_debug_turns_file.open(debug_turns_path);
        dg_debug_turns_file << "{\"type\":\"FeatureCollection\", \"features\":[" << std::endl;
    }
}

inline void DEBUG_SIGNAL(const NodeID node,
                         const std::vector<extractor::QueryNode> &m_node_info_list,
                         const int traffic_signal_penalty)
{
    if (dg_output_turn_debug)
    {
        const extractor::QueryNode &nodeinfo = m_node_info_list[node];
        if (!dg_first_turn_debug)
            dg_debug_turns_file << "," << std::endl;
        dg_debug_turns_file
            << "{ \"type\":\"Feature\",\"properties\":{\"type\":\"trafficlights\",\"cost\":"
            << traffic_signal_penalty / 10. << "},";
        dg_debug_turns_file << " \"geometry\":{\"type\":\"Point\",\"coordinates\":["
                            << std::setprecision(12) << nodeinfo.lon / COORDINATE_PRECISION << ","
                            << nodeinfo.lat / COORDINATE_PRECISION << "]}}";
        dg_first_turn_debug = false;
    }
}

inline void DEBUG_UTURN(const NodeID node,
                        const std::vector<extractor::QueryNode> &m_node_info_list,
                        const int traffic_signal_penalty)
{
    if (dg_output_turn_debug)
    {
        const extractor::QueryNode &nodeinfo = m_node_info_list[node];
        if (!dg_first_turn_debug)
            dg_debug_turns_file << "," << std::endl;
        dg_debug_turns_file
            << "{ \"type\":\"Feature\",\"properties\":{\"type\":\"trafficlights\",\"cost\":"
            << traffic_signal_penalty / 10. << "},";
        dg_debug_turns_file << " \"geometry\":{\"type\":\"Point\",\"coordinates\":["
                            << std::setprecision(12) << nodeinfo.lon / COORDINATE_PRECISION << ","
                            << nodeinfo.lat / COORDINATE_PRECISION << "]}}";
        dg_first_turn_debug = false;
    }
}

inline void DEBUG_TURN(const NodeID node,
                       const std::vector<extractor::QueryNode> &m_node_info_list,
                       const FixedPointCoordinate first_coordinate,
                       const int turn_angle,
                       const int turn_penalty)
{
    if (turn_penalty > 0 && dg_output_turn_debug)
    {
        const extractor::QueryNode &v = m_node_info_list[node];

        const float bearing_uv = coordinate_calculation::bearing(first_coordinate, v);
        float uvw_normal = bearing_uv + turn_angle / 2;
        while (uvw_normal >= 360.)
        {
            uvw_normal -= 360.;
        }

        if (!dg_first_turn_debug)
            dg_debug_turns_file << "," << std::endl;
        dg_debug_turns_file << "{ \"type\":\"Feature\",\"properties\":{\"type\":\"turn\",\"cost\":"
                            << turn_penalty / 10.
                            << ",\"turn_angle\":" << static_cast<int>(turn_angle)
                            << ",\"normal\":" << static_cast<int>(uvw_normal) << "},";
        dg_debug_turns_file << " \"geometry\":{\"type\":\"Point\",\"coordinates\":["
                            << std::setprecision(12) << v.lon / COORDINATE_PRECISION << ","
                            << v.lat / COORDINATE_PRECISION << "]}}";
        dg_first_turn_debug = false;
    }
}

inline void DEBUG_TURNS_STOP()
{
    if (dg_output_turn_debug)
    {
        dg_debug_turns_file << "\n]}" << std::endl;
        dg_debug_turns_file.close();
    }
}
}
}

#endif // DEBUG_GEOMETRY

#endif // DEBUG_GEOMETRY_H
