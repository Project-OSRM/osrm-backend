#ifndef OSRM_CUSTOMIZER_IO_HPP
#define OSRM_CUSTOMIZER_IO_HPP

#include "customizer/edge_based_graph.hpp"

#include "storage/io.hpp"

namespace osrm
{
namespace customizer
{
namespace io
{

inline void read(const boost::filesystem::path &path, StaticEdgeBasedGraph &graph)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    reader.DeserializeVector(graph.node_array);
    reader.DeserializeVector(graph.edge_array);
}

inline void write(const boost::filesystem::path &path, const StaticEdgeBasedGraph &graph)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    writer.SerializeVector(graph.node_array);
    writer.SerializeVector(graph.edge_array);
}
}
}
}

#endif
