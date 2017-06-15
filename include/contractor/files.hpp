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
void readCoreMarker(const boost::filesystem::path &path, CoreVectorT &is_core_node)
{
    static_assert(util::is_view_or_vector<bool, CoreVectorT>::value,
                  "is_core_node must be a vector");
    storage::io::FileReader reader(path, storage::io::FileReader::VerifyFingerprint);

    storage::serialization::read(reader, is_core_node);
}

// writes .osrm.core
template <typename CoreVectorT>
void writeCoreMarker(const boost::filesystem::path &path, const CoreVectorT &is_core_node)
{
    static_assert(util::is_view_or_vector<bool, CoreVectorT>::value,
                  "is_core_node must be a vector");
    storage::io::FileWriter writer(path, storage::io::FileWriter::GenerateFingerprint);

    storage::serialization::write(writer, is_core_node);
}

// reads .osrm.hsgr file
template <typename QueryGraphT>
inline void readGraph(const boost::filesystem::path &path, unsigned &checksum, QueryGraphT &graph)
{
    static_assert(std::is_same<QueryGraphView, QueryGraphT>::value ||
                      std::is_same<QueryGraph, QueryGraphT>::value,
                  "graph must be of type QueryGraph<>");

    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    reader.ReadInto(checksum);
    util::serialization::read(reader, graph);
}

// writes .osrm.hsgr file
template <typename QueryGraphT>
inline void
writeGraph(const boost::filesystem::path &path, unsigned checksum, const QueryGraphT &graph)
{
    static_assert(std::is_same<QueryGraphView, QueryGraphT>::value ||
                      std::is_same<QueryGraph, QueryGraphT>::value,
                  "graph must be of type QueryGraph<>");
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    writer.WriteOne(checksum);
    util::serialization::write(writer, graph);
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
