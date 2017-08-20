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
// reads .osrm.core
template <typename CoreVectorT>
void readCoreMarker(const boost::filesystem::path &path, std::vector<CoreVectorT> &cores)
{
    static_assert(util::is_view_or_vector<bool, CoreVectorT>::value,
                  "cores must be a vector of boolean vectors");
    storage::io::FileReader reader(path, storage::io::FileReader::VerifyFingerprint);

    auto num_cores = reader.ReadElementCount64();
    cores.resize(num_cores);
    for (const auto index : util::irange<std::size_t>(0, num_cores))
    {
        storage::serialization::read(reader, cores[index]);
    }
}

// writes .osrm.core
template <typename CoreVectorT>
void writeCoreMarker(const boost::filesystem::path &path, const std::vector<CoreVectorT> &cores)
{
    static_assert(util::is_view_or_vector<bool, CoreVectorT>::value,
                  "cores must be a vector of boolean vectors");
    storage::io::FileWriter writer(path, storage::io::FileWriter::GenerateFingerprint);

    writer.WriteElementCount64(cores.size());
    for (const auto &core : cores)
    {
        storage::serialization::write(writer, core);
    }
}

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

// reads .levels file
inline void readLevels(const boost::filesystem::path &path, std::vector<float> &node_levels)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, node_levels);
}

// writes .levels file
inline void writeLevels(const boost::filesystem::path &path, const std::vector<float> &node_levels)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, node_levels);
}
}
}
}

#endif
