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

template <>
void read(const boost::filesystem::path &path, SegmentDataContainer &segment_data)
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

    reader.ReadInto(segment_data.nodes.data(), segment_data.nodes.size());
    reader.ReadInto(segment_data.fwd_weights.data(), segment_data.fwd_weights.size());
    reader.ReadInto(segment_data.rev_weights.data(), segment_data.rev_weights.size());
    reader.ReadInto(segment_data.fwd_durations.data(), segment_data.fwd_durations.size());
    reader.ReadInto(segment_data.rev_durations.data(), segment_data.rev_durations.size());
}

template <> void write(const boost::filesystem::path &path, const SegmentDataContainer &segment_data)
{
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    // FIXME this _should_ just be size and the senitel below need to be removed
    writer.WriteElementCount32(segment_data.index.size() + 1);
    writer.WriteFrom(segment_data.index.data(), segment_data.index.size());
    // FIMXE remove unnecessary senitel
    writer.WriteElementCount32(segment_data.nodes.size());

    writer.WriteElementCount32(segment_data.nodes.size());
    writer.WriteFrom(segment_data.nodes.data(), segment_data.nodes.size());
    writer.WriteFrom(segment_data.fwd_weights.data(), segment_data.fwd_weights.size());
    writer.WriteFrom(segment_data.rev_weights.data(), segment_data.rev_weights.size());
    writer.WriteFrom(segment_data.fwd_durations.data(), segment_data.fwd_durations.size());
    writer.WriteFrom(segment_data.rev_durations.data(), segment_data.rev_durations.size());
}

}
}
}

#endif
