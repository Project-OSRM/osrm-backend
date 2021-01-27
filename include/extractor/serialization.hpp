#ifndef OSRM_EXTRACTOR_IO_HPP
#define OSRM_EXTRACTOR_IO_HPP

#include "extractor/conditional_turn_penalty.hpp"
#include "extractor/datasources.hpp"
#include "extractor/intersection_bearings_container.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/name_table.hpp"
#include "extractor/nbg_to_ebg.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/restriction.hpp"
#include "extractor/segment_data_container.hpp"

#include "storage/io.hpp"
#include "storage/serialization.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace serialization
{

// read/write for bearing data
template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::IntersectionBearingsContainer<Ownership> &intersection_bearings)
{
    storage::serialization::read(reader, name + "/bearing_values", intersection_bearings.values);
    storage::serialization::read(
        reader, name + "/node_to_class_id", intersection_bearings.node_to_class_id);
    util::serialization::read(
        reader, name + "/class_id_to_ranges", intersection_bearings.class_id_to_ranges_table);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::IntersectionBearingsContainer<Ownership> &intersection_bearings)
{
    storage::serialization::write(writer, name + "/bearing_values", intersection_bearings.values);
    storage::serialization::write(
        writer, name + "/node_to_class_id", intersection_bearings.node_to_class_id);
    util::serialization::write(
        writer, name + "/class_id_to_ranges", intersection_bearings.class_id_to_ranges_table);
}

// read/write for properties file
inline void
read(storage::tar::FileReader &reader, const std::string &name, ProfileProperties &properties)
{
    reader.ReadInto(name, properties);
}

inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const ProfileProperties &properties)
{
    writer.WriteElementCount64(name, 1);
    writer.WriteFrom(name, properties);
}

// read/write for datasources file
inline void read(storage::tar::FileReader &reader, const std::string &name, Datasources &sources)
{
    reader.ReadInto(name, sources);
}

inline void write(storage::tar::FileWriter &writer, const std::string &name, Datasources &sources)
{
    writer.WriteElementCount64(name, 1);
    writer.WriteFrom(name, sources);
}

// read/write for segment data file
template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::SegmentDataContainerImpl<Ownership> &segment_data)
{
    storage::serialization::read(reader, name + "/index", segment_data.index);
    storage::serialization::read(reader, name + "/nodes", segment_data.nodes);
    util::serialization::read(reader, name + "/forward_weights", segment_data.fwd_weights);
    util::serialization::read(reader, name + "/reverse_weights", segment_data.rev_weights);
    util::serialization::read(reader, name + "/forward_durations", segment_data.fwd_durations);
    util::serialization::read(reader, name + "/reverse_durations", segment_data.rev_durations);
    storage::serialization::read(
        reader, name + "/forward_data_sources", segment_data.fwd_datasources);
    storage::serialization::read(
        reader, name + "/reverse_data_sources", segment_data.rev_datasources);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::SegmentDataContainerImpl<Ownership> &segment_data)
{
    storage::serialization::write(writer, name + "/index", segment_data.index);
    storage::serialization::write(writer, name + "/nodes", segment_data.nodes);
    util::serialization::write(writer, name + "/forward_weights", segment_data.fwd_weights);
    util::serialization::write(writer, name + "/reverse_weights", segment_data.rev_weights);
    util::serialization::write(writer, name + "/forward_durations", segment_data.fwd_durations);
    util::serialization::write(writer, name + "/reverse_durations", segment_data.rev_durations);
    storage::serialization::write(
        writer, name + "/forward_data_sources", segment_data.fwd_datasources);
    storage::serialization::write(
        writer, name + "/reverse_data_sources", segment_data.rev_datasources);
}

template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::EdgeBasedNodeDataContainerImpl<Ownership> &node_data_container)
{
    // read actual data
    storage::serialization::read(reader, name + "/nodes", node_data_container.nodes);
    storage::serialization::read(
        reader, name + "/annotations", node_data_container.annotation_data);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::EdgeBasedNodeDataContainerImpl<Ownership> &node_data_container)
{
    storage::serialization::write(writer, name + "/nodes", node_data_container.nodes);
    storage::serialization::write(
        writer, name + "/annotations", node_data_container.annotation_data);
}

inline void read(storage::io::BufferReader &reader, ConditionalTurnPenalty &turn_penalty)
{
    reader.ReadInto(turn_penalty.turn_offset);
    reader.ReadInto(turn_penalty.location.lat);
    reader.ReadInto(turn_penalty.location.lon);
    auto const num_conditions = reader.ReadElementCount64();
    turn_penalty.conditions.resize(num_conditions);
    for (auto &condition : turn_penalty.conditions)
    {
        reader.ReadInto(condition.modifier);
        storage::serialization::read(reader, condition.times);
        storage::serialization::read(reader, condition.weekdays);
        storage::serialization::read(reader, condition.monthdays);
    }
}

inline void write(storage::io::BufferWriter &writer, const ConditionalTurnPenalty &turn_penalty)
{
    writer.WriteFrom(turn_penalty.turn_offset);
    writer.WriteFrom(static_cast<util::FixedLatitude::value_type>(turn_penalty.location.lat));
    writer.WriteFrom(static_cast<util::FixedLongitude::value_type>(turn_penalty.location.lon));
    writer.WriteElementCount64(turn_penalty.conditions.size());
    for (const auto &c : turn_penalty.conditions)
    {
        writer.WriteFrom(c.modifier);
        storage::serialization::write(writer, c.times);
        storage::serialization::write(writer, c.weekdays);
        storage::serialization::write(writer, c.monthdays);
    }
}

inline void write(storage::io::BufferWriter &writer,
                  const std::vector<ConditionalTurnPenalty> &conditional_penalties)
{
    writer.WriteElementCount64(conditional_penalties.size());
    for (const auto &penalty : conditional_penalties)
    {
        write(writer, penalty);
    }
}

inline void read(storage::io::BufferReader &reader,
                 std::vector<ConditionalTurnPenalty> &conditional_penalties)
{
    auto num_elements = reader.ReadElementCount64();
    conditional_penalties.resize(num_elements);
    for (auto &penalty : conditional_penalties)
    {
        read(reader, penalty);
    }
}

inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const std::vector<ConditionalTurnPenalty> &conditional_penalties)
{
    storage::io::BufferWriter buffer_writer;
    write(buffer_writer, conditional_penalties);

    storage::serialization::write(writer, name, buffer_writer.GetBuffer());
}

inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 std::vector<ConditionalTurnPenalty> &conditional_penalties)
{
    std::string buffer;
    storage::serialization::read(reader, name, buffer);

    storage::io::BufferReader buffer_reader{buffer};
    read(buffer_reader, conditional_penalties);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::NameTableImpl<Ownership> &name_table)
{
    storage::io::BufferWriter buffer_writer;
    util::serialization::write(writer, name, name_table.indexed_data);
}

template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::NameTableImpl<Ownership> &name_table)
{
    std::string buffer;
    util::serialization::read(reader, name, name_table.indexed_data);
}
} // namespace serialization
} // namespace extractor
} // namespace osrm

#endif
