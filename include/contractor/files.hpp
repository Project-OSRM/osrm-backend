#ifndef OSRM_CONTRACTOR_FILES_HPP
#define OSRM_CONTRACTOR_FILES_HPP

#include "contractor/query_graph.hpp"

#include "util/serialization.hpp"

#include "storage/io.hpp"
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
                      std::vector<EdgeFilterT> &edge_filter)
{
    static_assert(std::is_same<QueryGraphView, QueryGraphT>::value ||
                      std::is_same<QueryGraph, QueryGraphT>::value,
                  "graph must be of type QueryGraph<>");
    static_assert(std::is_same<EdgeFilterT, std::vector<bool>>::value ||
                      std::is_same<EdgeFilterT, util::vector_view<bool>>::value,
                  "edge_filter must be a container of vector<bool> or vector_view<bool>");

    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    reader.ReadInto(checksum);
    util::serialization::read(reader, graph);
    auto count = reader.ReadElementCount64();
    edge_filter.resize(count);
    for (const auto index : util::irange<std::size_t>(0, count))
    {
        storage::serialization::read(reader, edge_filter[index]);
    }
}

// writes .osrm.hsgr file
template <typename QueryGraphT, typename EdgeFilterT>
inline void writeGraph(const boost::filesystem::path &path,
                       unsigned checksum,
                       const QueryGraphT &graph,
                       const std::vector<EdgeFilterT> &edge_filter)
{
    static_assert(std::is_same<QueryGraphView, QueryGraphT>::value ||
                      std::is_same<QueryGraph, QueryGraphT>::value,
                  "graph must be of type QueryGraph<>");
    static_assert(std::is_same<EdgeFilterT, std::vector<bool>>::value ||
                      std::is_same<EdgeFilterT, util::vector_view<bool>>::value,
                  "edge_filter must be a container of vector<bool> or vector_view<bool>");
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    writer.WriteOne(checksum);
    util::serialization::write(writer, graph);
    writer.WriteElementCount64(edge_filter.size());
    for (const auto &filter : edge_filter)
    {
        storage::serialization::write(writer, filter);
    }
}
}
}
}

#endif
