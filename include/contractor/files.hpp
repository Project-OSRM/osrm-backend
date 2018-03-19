#ifndef OSRM_CONTRACTOR_FILES_HPP
#define OSRM_CONTRACTOR_FILES_HPP

#include "contractor/query_graph.hpp"

#include "util/serialization.hpp"

#include "storage/tar.hpp"
#include "storage/serialization.hpp"

namespace osrm
{
namespace contractor
{
namespace files
{
// reads .osrm.hsgr file
template <typename QueryGraphT, typename EdgeFilterT>
inline void readGraph(const boost::filesystem::path &path,
                      unsigned &checksum,
                      QueryGraphT &graph,
                      std::vector<EdgeFilterT> &edge_filter,
                      std::uint32_t &connectivity_checksum)
{
    static_assert(std::is_same<QueryGraphView, QueryGraphT>::value ||
                      std::is_same<QueryGraph, QueryGraphT>::value,
                  "graph must be of type QueryGraph<>");
    static_assert(std::is_same<EdgeFilterT, std::vector<bool>>::value ||
                      std::is_same<EdgeFilterT, util::vector_view<bool>>::value,
                  "edge_filter must be a container of vector<bool> or vector_view<bool>");

    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    reader.ReadInto("/ch/checksum", checksum);
    util::serialization::read(reader, "/ch/contracted_graph", graph);

    auto count = reader.ReadElementCount64("/ch/edge_filter");
    edge_filter.resize(count);
    for (const auto index : util::irange<std::size_t>(0, count))
    {
        storage::serialization::read(reader, "/ch/edge_filter/" + std::to_string(index), edge_filter[index]);
    }

     reader.ReadInto("/ch/connectivity_checksum", connectivity_checksum);
}

// writes .osrm.hsgr file
template <typename QueryGraphT, typename EdgeFilterT>
inline void writeGraph(const boost::filesystem::path &path,
                       unsigned checksum,
                       const QueryGraphT &graph,
                       const std::vector<EdgeFilterT> &edge_filter,
                       const std::uint32_t connectivity_checksum)
{
    static_assert(std::is_same<QueryGraphView, QueryGraphT>::value ||
                      std::is_same<QueryGraph, QueryGraphT>::value,
                  "graph must be of type QueryGraph<>");
    static_assert(std::is_same<EdgeFilterT, std::vector<bool>>::value ||
                      std::is_same<EdgeFilterT, util::vector_view<bool>>::value,
                  "edge_filter must be a container of vector<bool> or vector_view<bool>");
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    writer.WriteElementCount64("/ch/checksum", 1);
    writer.WriteFrom("/ch/checksum", checksum);
    util::serialization::write(writer, "/ch/contracted_graph", graph);

    writer.WriteElementCount64("/ch/edge_filter", edge_filter.size());
    for (const auto index : util::irange<std::size_t>(0, edge_filter.size()))
    {
        storage::serialization::write(writer, "/ch/edge_filter/" + std::to_string(index), edge_filter[index]);
    }

    writer.WriteElementCount64("/ch/connectivity_checksum", 1);
    writer.WriteFrom("/ch/connectivity_checksum", connectivity_checksum);
}
}
}
}

#endif
