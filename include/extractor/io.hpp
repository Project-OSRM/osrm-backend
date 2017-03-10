#ifndef OSRM_EXTRACTOR_IO_HPP
#define OSRM_EXTRACTOR_IO_HPP

#include "extractor/segment_data_container.hpp"

#include "storage/io.hpp"

namespace osrm
{
namespace extractor
{
namespace io
{

template <> void read(const boost::filesystem::path &path, SegmentDataContainer &segment_data)
{
    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    auto num_indices = reader.ReadElementCount32();
    segment_data.index.resize(num_indices);
    reader.ReadInto(segment_data.index.data(), num_indices);

    auto num_entries = reader.ReadElementCount32();
    segment_data.nodes.resize(num_entries);
    segment_data.fwd_weights.resize(num_entries);
    segment_data.rev_weights.resize(num_entries);
    segment_data.fwd_durations.resize(num_entries);
    segment_data.rev_durations.resize(num_entries);

    reader.ReadInto(segment_data.nodes);
    reader.ReadInto(segment_data.fwd_weights);
    reader.ReadInto(segment_data.rev_weights);
    reader.ReadInto(segment_data.fwd_durations);
    reader.ReadInto(segment_data.rev_durations);
}

template <>
void write(const boost::filesystem::path &path, const SegmentDataContainer &segment_data)
{
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    writer.WriteElementCount32(segment_data.index.size());
    writer.WriteFrom(segment_data.index);

    writer.WriteElementCount32(segment_data.nodes.size());
    writer.WriteFrom(segment_data.nodes);
    writer.WriteFrom(segment_data.fwd_weights);
    writer.WriteFrom(segment_data.rev_weights);
    writer.WriteFrom(segment_data.fwd_durations);
    writer.WriteFrom(segment_data.rev_durations);
}
}
}
}

#endif
