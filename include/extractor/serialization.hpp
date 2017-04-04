#ifndef OSRM_EXTRACTOR_IO_HPP
#define OSRM_EXTRACTOR_IO_HPP

#include "extractor/datasources.hpp"
#include "extractor/nbg_to_ebg.hpp"
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

template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 detail::TurnDataContainerImpl<Ownership> &turn_data_container)
{
    storage::serialization::read(reader, turn_data_container.geometry_ids);
    storage::serialization::read(reader, turn_data_container.name_ids);
    storage::serialization::read(reader, turn_data_container.turn_instructions);
    storage::serialization::read(reader, turn_data_container.lane_data_ids);
    storage::serialization::read(reader, turn_data_container.travel_modes);
    storage::serialization::read(reader, turn_data_container.entry_class_ids);
    storage::serialization::read(reader, turn_data_container.pre_turn_bearings);
    storage::serialization::read(reader, turn_data_container.post_turn_bearings);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::TurnDataContainerImpl<Ownership> &turn_data_container)
{
    storage::serialization::write(writer, turn_data_container.geometry_ids);
    storage::serialization::write(writer, turn_data_container.name_ids);
    storage::serialization::write(writer, turn_data_container.turn_instructions);
    storage::serialization::write(writer, turn_data_container.lane_data_ids);
    storage::serialization::write(writer, turn_data_container.travel_modes);
    storage::serialization::write(writer, turn_data_container.entry_class_ids);
    storage::serialization::write(writer, turn_data_container.pre_turn_bearings);
    storage::serialization::write(writer, turn_data_container.post_turn_bearings);
}
}
}
}

#endif
