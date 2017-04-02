#ifndef OSRM_EXTRACTOR_IO_HPP
#define OSRM_EXTRACTOR_IO_HPP

#include "extractor/datasources.hpp"
#include "extractor/nbg_to_ebg.hpp"
#include "extractor/segment_data_container.hpp"
#include "extractor/turn_data_container.hpp"

#include "storage/io.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace serialization
{

inline void read(storage::io::FileReader &reader, std::vector<NBGToEBG> &mapping)
{
    reader.DeserializeVector(mapping);
}

inline void write(storage::io::FileWriter &writer, const std::vector<NBGToEBG> &mapping)
{
    writer.SerializeVector(mapping);
}

inline void read(storage::io::FileReader &reader, Datasources &sources)
{
    reader.ReadInto(sources);
}

inline void write(storage::io::FileWriter &writer, Datasources &sources)
{
    writer.WriteFrom(sources);
}

template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 detail::SegmentDataContainerImpl<Ownership> &segment_data)
{
    reader.DeserializeVector(segment_data.index);
    reader.DeserializeVector(segment_data.nodes);
    reader.DeserializeVector(segment_data.fwd_weights);
    reader.DeserializeVector(segment_data.rev_weights);
    reader.DeserializeVector(segment_data.fwd_durations);
    reader.DeserializeVector(segment_data.rev_durations);
    reader.DeserializeVector(segment_data.datasources);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::SegmentDataContainerImpl<Ownership> &segment_data)
{
    writer.SerializeVector(segment_data.index);
    writer.SerializeVector(segment_data.nodes);
    writer.SerializeVector(segment_data.fwd_weights);
    writer.SerializeVector(segment_data.rev_weights);
    writer.SerializeVector(segment_data.fwd_durations);
    writer.SerializeVector(segment_data.rev_durations);
    writer.SerializeVector(segment_data.datasources);
}

template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 detail::TurnDataContainerImpl<Ownership> &turn_data_container)
{
    reader.DeserializeVector(turn_data_container.geometry_ids);
    reader.DeserializeVector(turn_data_container.name_ids);
    reader.DeserializeVector(turn_data_container.turn_instructions);
    reader.DeserializeVector(turn_data_container.lane_data_ids);
    reader.DeserializeVector(turn_data_container.travel_modes);
    reader.DeserializeVector(turn_data_container.entry_class_ids);
    reader.DeserializeVector(turn_data_container.pre_turn_bearings);
    reader.DeserializeVector(turn_data_container.post_turn_bearings);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::TurnDataContainerImpl<Ownership> &turn_data_container)
{
    writer.SerializeVector(turn_data_container.geometry_ids);
    writer.SerializeVector(turn_data_container.name_ids);
    writer.SerializeVector(turn_data_container.turn_instructions);
    writer.SerializeVector(turn_data_container.lane_data_ids);
    writer.SerializeVector(turn_data_container.travel_modes);
    writer.SerializeVector(turn_data_container.entry_class_ids);
    writer.SerializeVector(turn_data_container.pre_turn_bearings);
    writer.SerializeVector(turn_data_container.post_turn_bearings);
}
}
}
}

#endif
