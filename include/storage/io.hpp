#ifndef OSRM_STORAGE_IO_HPP_
#define OSRM_STORAGE_IO_HPP_

#include "contractor/query_edge.hpp"
#include "extractor/extractor.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/query_node.hpp"
#include "util/fingerprint.hpp"
#include "util/simple_logger.hpp"
#include "util/static_graph.hpp"
#include "util/exception.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/seek.hpp>

#include <tuple>
#include <cstring>
#include <cerrno>

namespace osrm
{
namespace storage
{
namespace io
{

class FileReader
{
  private:
    const std::string filename;
    boost::filesystem::ifstream input_stream;

  public:
    FileReader(const std::string &filename, const bool check_fingerprint = false)
        : FileReader(boost::filesystem::path(filename), check_fingerprint)
    {
    }

    FileReader(const boost::filesystem::path &filename_, const bool check_fingerprint = false)
        : filename(filename_.string())
    {
        input_stream.open(filename_, std::ios::binary);
        if (!input_stream)
            throw util::exception("Error opening " + filename + ":" + std::strerror(errno));

        if (check_fingerprint && !ReadAndCheckFingerprint())
        {
            throw util::exception("Fingerprint mismatch in " + filename);
        }
    }

    /* Read count objects of type T into pointer dest */
    template <typename T> void ReadInto(T *dest, const std::size_t count)
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "bytewise reading requires trivially copyable type");
        if (count == 0)
            return;

        const auto &result = input_stream.read(reinterpret_cast<char *>(dest), count * sizeof(T));
        if (!result)
        {
            if (result.eof())
            {
                throw util::exception("Error reading from " + filename +
                                      ": Unexpected end of file");
            }
            throw util::exception("Error reading from " + filename + ": " + std::strerror(errno));
        }
    }

    template <typename T> void ReadInto(T &target) { ReadInto(&target, 1); }

    template <typename T> T ReadOne()
    {
        T tmp;
        ReadInto(tmp);
        return tmp;
    }

    template <typename T> void Skip(const std::size_t element_count)
    {
        boost::iostreams::seek(input_stream, element_count * sizeof(T), BOOST_IOS::cur);
    }

    /*******************************************/

    std::uint32_t ReadElementCount32() { return ReadOne<std::uint32_t>(); }
    std::uint64_t ReadElementCount64() { return ReadOne<std::uint64_t>(); }

    template <typename T> void DeserializeVector(std::vector<T> &data)
    {
        const auto count = ReadElementCount64();
        data.resize(count);
        ReadInto(data.data(), count);
    }

    bool ReadAndCheckFingerprint()
    {
        auto fingerprint = ReadOne<util::FingerPrint>();
        const auto valid = util::FingerPrint::GetValid();
        // compare the compilation state stored in the fingerprint
        return valid.IsMagicNumberOK(fingerprint) && valid.TestContractor(fingerprint) &&
               valid.TestGraphUtil(fingerprint) && valid.TestRTree(fingerprint) &&
               valid.TestQueryObjects(fingerprint);
    }

    std::size_t Size()
    {
        auto current_pos = input_stream.tellg();
        input_stream.seekg(0, input_stream.end);
        auto length = input_stream.tellg();
        input_stream.seekg(current_pos, input_stream.beg);
        return length;
    }

    std::vector<std::string> ReadLines()
    {
        std::vector<std::string> result;
        std::string thisline;
        try
        {
            while (std::getline(input_stream, thisline))
            {
                result.push_back(thisline);
            }
        }
        catch (const std::ios_base::failure &e)
        {
            // EOF is OK here, everything else, re-throw
            if (!input_stream.eof())
                throw;
        }
        return result;
    }
};

// To make function calls consistent, this function returns the fixed number of properties
inline std::size_t readPropertiesCount() { return 1; }

#pragma pack(push, 1)
struct HSGRHeader
{
    std::uint32_t checksum;
    std::uint64_t number_of_nodes;
    std::uint64_t number_of_edges;
};
#pragma pack(pop)
static_assert(sizeof(HSGRHeader) == 20, "HSGRHeader is not packed");

// Reads the checksum, number of nodes and number of edges written in the header file of a `.hsgr`
// file and returns them in a HSGRHeader struct
inline HSGRHeader readHSGRHeader(io::FileReader &input_file)
{
    const util::FingerPrint fingerprint_valid = util::FingerPrint::GetValid();
    const auto fingerprint_loaded = input_file.ReadOne<util::FingerPrint>();
    if (!fingerprint_loaded.TestGraphUtil(fingerprint_valid))
    {
        util::SimpleLogger().Write(logWARNING) << ".hsgr was prepared with different build.\n"
                                                  "Reprocess to get rid of this warning.";
    }

    HSGRHeader header;
    input_file.ReadInto(header.checksum);
    input_file.ReadInto(header.number_of_nodes);
    input_file.ReadInto(header.number_of_edges);

    BOOST_ASSERT_MSG(0 != header.number_of_nodes, "number of nodes is zero");
    // number of edges can be zero, this is the case in a few test fixtures

    return header;
}

// Reads the graph data of a `.hsgr` file into memory
// Needs to be called after readHSGRHeader() to get the correct offset in the stream
using NodeT = typename util::StaticGraph<contractor::QueryEdge::EdgeData>::NodeArrayEntry;
using EdgeT = typename util::StaticGraph<contractor::QueryEdge::EdgeData>::EdgeArrayEntry;
inline void readHSGR(io::FileReader &input_file,
                     NodeT *node_buffer,
                     const std::uint64_t number_of_nodes,
                     EdgeT *edge_buffer,
                     const std::uint64_t number_of_edges)
{
    BOOST_ASSERT(node_buffer);
    BOOST_ASSERT(edge_buffer);
    input_file.ReadInto(node_buffer, number_of_nodes);
    input_file.ReadInto(edge_buffer, number_of_edges);
}

// Loads datasource_indexes from .datasource_indexes into memory
// Needs to be called after readElementCount() to get the correct offset in the stream
inline void readDatasourceIndexes(io::FileReader &datasource_indexes_file,
                                  uint8_t *datasource_buffer,
                                  const std::uint64_t number_of_datasource_indexes)
{
    BOOST_ASSERT(datasource_buffer);
    datasource_indexes_file.ReadInto(datasource_buffer, number_of_datasource_indexes);
}

// Loads edge data from .edge files into memory which includes its
// geometry, name ID, turn instruction, lane data ID, travel mode, entry class ID
// Needs to be called after readElementCount() to get the correct offset in the stream
inline void readEdges(io::FileReader &edges_input_file,
                      GeometryID *geometry_list,
                      NameID *name_id_list,
                      extractor::guidance::TurnInstruction *turn_instruction_list,
                      LaneDataID *lane_data_id_list,
                      extractor::TravelMode *travel_mode_list,
                      EntryClassID *entry_class_id_list,
                      util::guidance::TurnBearing *pre_turn_bearing_list,
                      util::guidance::TurnBearing *post_turn_bearing_list,
                      const std::uint64_t number_of_edges)
{
    BOOST_ASSERT(geometry_list);
    BOOST_ASSERT(name_id_list);
    BOOST_ASSERT(turn_instruction_list);
    BOOST_ASSERT(lane_data_id_list);
    BOOST_ASSERT(travel_mode_list);
    BOOST_ASSERT(entry_class_id_list);
    extractor::OriginalEdgeData current_edge_data;
    for (std::uint64_t i = 0; i < number_of_edges; ++i)
    {
        edges_input_file.ReadInto(current_edge_data);

        geometry_list[i] = current_edge_data.via_geometry;
        name_id_list[i] = current_edge_data.name_id;
        turn_instruction_list[i] = current_edge_data.turn_instruction;
        lane_data_id_list[i] = current_edge_data.lane_data_id;
        travel_mode_list[i] = current_edge_data.travel_mode;
        entry_class_id_list[i] = current_edge_data.entry_classid;
        pre_turn_bearing_list[i] = current_edge_data.pre_turn_bearing;
        post_turn_bearing_list[i] = current_edge_data.post_turn_bearing;
    }
}

// Loads coordinates and OSM node IDs from .nodes files into memory
// Needs to be called after readElementCount() to get the correct offset in the stream
template <typename OSMNodeIDVectorT>
void readNodes(io::FileReader &nodes_file,
               util::Coordinate *coordinate_list,
               OSMNodeIDVectorT &osmnodeid_list,
               const std::uint64_t number_of_coordinates)
{
    BOOST_ASSERT(coordinate_list);
    extractor::QueryNode current_node;
    for (std::uint64_t i = 0; i < number_of_coordinates; ++i)
    {
        nodes_file.ReadInto(current_node);
        coordinate_list[i] = util::Coordinate(current_node.lon, current_node.lat);
        osmnodeid_list.push_back(current_node.node_id);
        BOOST_ASSERT(coordinate_list[i].IsValid());
    }
}

// Reads datasource names out of .datasource_names files and metadata such as
// the length and offset of each name
struct DatasourceNamesData
{
    std::vector<char> names;
    std::vector<std::size_t> offsets;
    std::vector<std::size_t> lengths;
};
inline DatasourceNamesData readDatasourceNames(io::FileReader &datasource_names_file)
{
    DatasourceNamesData datasource_names_data;
    std::vector<std::string> lines = datasource_names_file.ReadLines();
    for (const auto &name : lines)
    {
        datasource_names_data.offsets.push_back(datasource_names_data.names.size());
        datasource_names_data.lengths.push_back(name.size());
        std::copy(name.c_str(),
                  name.c_str() + name.size(),
                  std::back_inserter(datasource_names_data.names));
    }
    return datasource_names_data;
}
}
}
}

#endif
