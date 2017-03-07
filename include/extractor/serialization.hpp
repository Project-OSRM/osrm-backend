#ifndef OSRM_EXTRACTOR_IO_HPP
#define OSRM_EXTRACTOR_IO_HPP

#include "extractor/datasources.hpp"
#include "extractor/nbg_to_ebg.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/restriction.hpp"
#include "extractor/segment_data_container.hpp"
#include "extractor/turn_data_container.hpp"

#include "storage/io.hpp"
#include "storage/serialization.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace serialization
{

// read/write for datasources file
inline void read(storage::io::FileReader &reader, Datasources &sources)
{
    reader.ReadInto(sources);
}

inline void write(storage::io::FileWriter &writer, Datasources &sources)
{
    writer.WriteFrom(sources);
}

// read/write for segment data file
template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 detail::SegmentDataContainerImpl<Ownership> &segment_data)
{
    storage::serialization::read(reader, segment_data.index);
    storage::serialization::read(reader, segment_data.nodes);
    storage::serialization::read(reader, segment_data.fwd_weights);
    storage::serialization::read(reader, segment_data.rev_weights);
    storage::serialization::read(reader, segment_data.fwd_durations);
    storage::serialization::read(reader, segment_data.rev_durations);
    storage::serialization::read(reader, segment_data.datasources);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::SegmentDataContainerImpl<Ownership> &segment_data)
{
    storage::serialization::write(writer, segment_data.index);
    storage::serialization::write(writer, segment_data.nodes);
    storage::serialization::write(writer, segment_data.fwd_weights);
    storage::serialization::write(writer, segment_data.rev_weights);
    storage::serialization::write(writer, segment_data.fwd_durations);
    storage::serialization::write(writer, segment_data.rev_durations);
    storage::serialization::write(writer, segment_data.datasources);
}

// read/write for turn data file
template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 detail::TurnDataContainerImpl<Ownership> &turn_data_container)
{
    storage::serialization::read(reader, turn_data_container.turn_instructions);
    storage::serialization::read(reader, turn_data_container.lane_data_ids);
    storage::serialization::read(reader, turn_data_container.entry_class_ids);
    storage::serialization::read(reader, turn_data_container.pre_turn_bearings);
    storage::serialization::read(reader, turn_data_container.post_turn_bearings);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::TurnDataContainerImpl<Ownership> &turn_data_container)
{
    storage::serialization::write(writer, turn_data_container.turn_instructions);
    storage::serialization::write(writer, turn_data_container.lane_data_ids);
    storage::serialization::write(writer, turn_data_container.entry_class_ids);
    storage::serialization::write(writer, turn_data_container.pre_turn_bearings);
    storage::serialization::write(writer, turn_data_container.post_turn_bearings);
}

template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 detail::EdgeBasedNodeDataContainerImpl<Ownership> &node_data_container)
{
    storage::serialization::read(reader, node_data_container.geometry_ids);
    storage::serialization::read(reader, node_data_container.name_ids);
    storage::serialization::read(reader, node_data_container.travel_modes);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::EdgeBasedNodeDataContainerImpl<Ownership> &node_data_container)
{
    storage::serialization::write(writer, node_data_container.geometry_ids);
    storage::serialization::write(writer, node_data_container.name_ids);
    storage::serialization::write(writer, node_data_container.travel_modes);
}

// read/write for conditional turn restrictions file
inline void read(const boost::filesystem::path &path, std::vector<InputRestrictionContainer> &restrictions)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    auto num_indices = reader.ReadElementCount64();
    restrictions.resize(num_indices);
    while (num_indices > 0)
    {
        InputRestrictionContainer restriction;
        bool is_only;
        reader.ReadInto(restriction.restriction.via);
        reader.ReadInto(restriction.restriction.from);
        reader.ReadInto(restriction.restriction.to);
        reader.ReadInto(is_only);
        reader.ReadInto(restriction.restriction.condition);
        restriction.restriction.flags.is_only = is_only;

        restrictions.push_back(restriction);
        num_indices--;
    }
}

inline void write(storage::io::FileWriter &writer, const InputRestrictionContainer &container)
{
    writer.WriteOne(container.restriction.via);
    writer.WriteOne(container.restriction.from);
    writer.WriteOne(container.restriction.to);
    writer.WriteOne(container.restriction.flags.is_only);
    // condition is a string
    writer.WriteFrom(container.restriction.condition);
}
}
}
}

#endif
