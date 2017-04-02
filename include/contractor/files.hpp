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
template<storage::Ownership Ownership>
inline void readGraph(const boost::filesystem::path &path,
                      unsigned &checksum,
                      detail::QueryGraph<Ownership> &graph)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    reader.ReadInto(checksum);
    util::serialization::read(reader, graph);
}

// writes .osrm.hsgr file
template<storage::Ownership Ownership>
inline void writeGraph(const boost::filesystem::path &path,
                       unsigned checksum,
                       const detail::QueryGraph<Ownership> &graph)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    writer.WriteOne(checksum);
    util::serialization::write(writer, graph);
}

}
}
}

#endif
