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
    storage::serialization::read(reader, node_data_container.component_ids);
    storage::serialization::read(reader, node_data_container.travel_modes);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::EdgeBasedNodeDataContainerImpl<Ownership> &node_data_container)
{
    storage::serialization::write(writer, node_data_container.geometry_ids);
    storage::serialization::write(writer, node_data_container.name_ids);
    storage::serialization::write(writer, node_data_container.component_ids);
    storage::serialization::write(writer, node_data_container.travel_modes);
}

// read/write for conditional turn restrictions file
inline void read(storage::io::FileReader &reader, std::vector<TurnRestriction> &restrictions)
{
    auto num_indices = reader.ReadElementCount64();
    restrictions.reserve(num_indices);
    TurnRestriction restriction;
    while (num_indices > 0)
    {
        bool is_only;
        reader.ReadInto(restriction.via);
        reader.ReadInto(restriction.from);
        reader.ReadInto(restriction.to);
        reader.ReadInto(is_only);
        auto num_conditions = reader.ReadElementCount64();
        restriction.condition.resize(num_conditions);
        for (uint64_t i = 0; i < num_conditions; i++)
        {
            reader.ReadInto(restriction.condition[i].modifier);
            storage::serialization::read(reader, restriction.condition[i].times);
            storage::serialization::read(reader, restriction.condition[i].weekdays);
            storage::serialization::read(reader, restriction.condition[i].monthdays);
        }
        restriction.flags.is_only = is_only;

        restrictions.push_back(std::move(restriction));
        num_indices--;
    }
}

inline void write(storage::io::FileWriter &writer, const TurnRestriction &restriction)
{
    writer.WriteOne(restriction.via);
    writer.WriteOne(restriction.from);
    writer.WriteOne(restriction.to);
    writer.WriteOne(restriction.flags.is_only);
    writer.WriteElementCount64(restriction.condition.size());
    for (const auto &c : restriction.condition)
    {
        writer.WriteOne(c.modifier);
        storage::serialization::write(writer, c.times);
        storage::serialization::write(writer, c.weekdays);
        storage::serialization::write(writer, c.monthdays);
    }
}
}
}
}

#endif
