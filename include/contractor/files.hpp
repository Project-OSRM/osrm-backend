#ifndef OSRM_CONTRACTOR_FILES_HPP
#define OSRM_CONTRACTOR_FILES_HPP

#include "contractor/query_graph.hpp"

#include "util/serialization.hpp"

#include "storage/io.hpp"

namespace osrm
{
namespace contractor
{
namespace files
{

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
}
}
}

#endif
