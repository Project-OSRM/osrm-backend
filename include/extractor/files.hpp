#ifndef OSRM_EXTRACTOR_FILES_HPP
#define OSRM_EXTRACTOR_FILES_HPP

#include "extractor/seralization.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace files
{

// reads .osrm.cnbg_to_ebg
inline void readNBGMapping(const boost::filesystem::path &path, std::vector<NBGToEBG> &mapping)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, mapping);
}

// writes .osrm.cnbg_to_ebg
inline void writeNBGMapping(const boost::filesystem::path &path, const std::vector<NBGToEBG> &mapping)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, mapping);
}

// reads .osrm.datasource_names
inline void readDatasources(const boost::filesystem::path &path, Datasources &sources)
{
    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, sources);
}

// writes .osrm.datasource_names
inline void writeDatasources(const boost::filesystem::path &path, Datasources &sources)
{
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, sources);
}

// reads .osrm.geometry
inline void readSegmentData(const boost::filesystem::path &path, SegmentDataContainer &segment_data)
{
    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, segment_data);
}

// writes .osrm.geometry
inline void writeSegmentData(const boost::filesystem::path &path, const SegmentDataContainer &segment_data)
{
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, segment_data);
}
}
}
}

#endif
