#ifndef OSRM_EXTRACTOR_IO_HPP
#define OSRM_EXTRACTOR_IO_HPP

#include "conditional_turn_penalty.hpp"
#include "extractor/datasources.hpp"
#include "extractor/intersection_bearings_container.hpp"
#include "extractor/nbg_to_ebg.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/profile_properties.hpp"
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

// read/write for bearing data
template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 detail::IntersectionBearingsContainer<Ownership> &intersection_bearings)
{
    storage::serialization::read(reader, intersection_bearings.values);
    storage::serialization::read(reader, intersection_bearings.node_to_class_id);
    util::serialization::read(reader, intersection_bearings.class_id_to_ranges_table);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::IntersectionBearingsContainer<Ownership> &intersection_bearings)
{
    storage::serialization::write(writer, intersection_bearings.values);
    storage::serialization::write(writer, intersection_bearings.node_to_class_id);
    util::serialization::write(writer, intersection_bearings.class_id_to_ranges_table);
}

// read/write for properties file
inline void read(storage::io::FileReader &reader, ProfileProperties &properties)
{
    reader.ReadInto(properties);
}

inline void write(storage::io::FileWriter &writer, const ProfileProperties &properties)
{
    writer.WriteFrom(properties);
}

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
    util::serialization::read(reader, segment_data.fwd_weights);
    util::serialization::read(reader, segment_data.rev_weights);
    util::serialization::read(reader, segment_data.fwd_durations);
    util::serialization::read(reader, segment_data.rev_durations);
    storage::serialization::read(reader, segment_data.fwd_datasources);
    storage::serialization::read(reader, segment_data.rev_datasources);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::SegmentDataContainerImpl<Ownership> &segment_data)
{
    storage::serialization::write(writer, segment_data.index);
    storage::serialization::write(writer, segment_data.nodes);
    util::serialization::write(writer, segment_data.fwd_weights);
    util::serialization::write(writer, segment_data.rev_weights);
    util::serialization::write(writer, segment_data.fwd_durations);
    util::serialization::write(writer, segment_data.rev_durations);
    storage::serialization::write(writer, segment_data.fwd_datasources);
    storage::serialization::write(writer, segment_data.rev_datasources);
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
    storage::serialization::read(reader, node_data_container.classes);
    storage::serialization::read(reader, node_data_container.is_left_hand_driving);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::EdgeBasedNodeDataContainerImpl<Ownership> &node_data_container)
{
    storage::serialization::write(writer, node_data_container.geometry_ids);
    storage::serialization::write(writer, node_data_container.name_ids);
    storage::serialization::write(writer, node_data_container.component_ids);
    storage::serialization::write(writer, node_data_container.travel_modes);
    storage::serialization::write(writer, node_data_container.classes);
    storage::serialization::write(writer, node_data_container.is_left_hand_driving);
}

inline void read(storage::io::FileReader &reader, NodeRestriction &restriction)
{
    reader.ReadInto(restriction.from);
    reader.ReadInto(restriction.via);
    reader.ReadInto(restriction.to);
}

inline void write(storage::io::FileWriter &writer, const NodeRestriction &restriction)
{
    writer.WriteOne(restriction.from);
    writer.WriteOne(restriction.via);
    writer.WriteOne(restriction.to);
}

inline void read(storage::io::FileReader &reader, WayRestriction &restriction)
{
    read(reader, restriction.in_restriction);
    read(reader, restriction.out_restriction);
}

inline void write(storage::io::FileWriter &writer, const WayRestriction &restriction)
{
    write(writer, restriction.in_restriction);
    write(writer, restriction.out_restriction);
}

inline void read(storage::io::FileReader &reader, TurnRestriction &restriction)
{
    reader.ReadInto(restriction.is_only);
    std::uint32_t restriction_type;
    reader.ReadInto(restriction_type);
    if (restriction_type == RestrictionType::WAY_RESTRICTION)
    {
        WayRestriction way_restriction;
        read(reader, way_restriction);
        restriction.node_or_way = std::move(way_restriction);
    }
    else
    {
        BOOST_ASSERT(restriction_type == RestrictionType::NODE_RESTRICTION);
        NodeRestriction node_restriction;
        read(reader, node_restriction);
        restriction.node_or_way = std::move(node_restriction);
    }
}

inline void write(storage::io::FileWriter &writer, const TurnRestriction &restriction)
{
    writer.WriteOne(restriction.is_only);
    const std::uint32_t restriction_type = restriction.Type();
    writer.WriteOne(restriction_type);
    if (restriction.Type() == RestrictionType::WAY_RESTRICTION)
    {
        write(writer, mapbox::util::get<WayRestriction>(restriction.node_or_way));
    }
    else
    {
        BOOST_ASSERT(restriction.Type() == RestrictionType::NODE_RESTRICTION);
        write(writer, mapbox::util::get<NodeRestriction>(restriction.node_or_way));
    }
}

inline void write(storage::io::FileWriter &writer, const ConditionalTurnRestriction &restriction)
{
    write(writer, static_cast<const TurnRestriction &>(restriction));
    writer.WriteElementCount64(restriction.condition.size());
    for (const auto &c : restriction.condition)
    {
        writer.WriteOne(c.modifier);
        storage::serialization::write(writer, c.times);
        storage::serialization::write(writer, c.weekdays);
        storage::serialization::write(writer, c.monthdays);
    }
}

inline void read(storage::io::FileReader &reader, ConditionalTurnRestriction &restriction)
{
    read(reader, static_cast<TurnRestriction &>(restriction));
    const auto num_conditions = reader.ReadElementCount64();
    restriction.condition.resize(num_conditions);
    for (uint64_t i = 0; i < num_conditions; i++)
    {
        reader.ReadInto(restriction.condition[i].modifier);
        storage::serialization::read(reader, restriction.condition[i].times);
        storage::serialization::read(reader, restriction.condition[i].weekdays);
        storage::serialization::read(reader, restriction.condition[i].monthdays);
    }
}

// read/write for conditional turn restrictions file
inline void read(storage::io::FileReader &reader, std::vector<TurnRestriction> &restrictions)
{
    auto num_indices = reader.ReadElementCount64();
    restrictions.reserve(num_indices);
    TurnRestriction restriction;
    while (num_indices-- > 0)
    {
        read(reader, restriction);
        restrictions.push_back(std::move(restriction));
    }
}

inline void write(storage::io::FileWriter &writer, const std::vector<TurnRestriction> &restrictions)
{
    const auto num_indices = restrictions.size();
    writer.WriteElementCount64(num_indices);
    const auto write_restriction = [&writer](const auto &restriction) {
        write(writer, restriction);
    };
    std::for_each(restrictions.begin(), restrictions.end(), write_restriction);
}

// read/write for conditional turn restrictions file
inline void read(storage::io::FileReader &reader,
                 std::vector<ConditionalTurnRestriction> &restrictions)
{
    auto num_indices = reader.ReadElementCount64();
    restrictions.reserve(num_indices);
    ConditionalTurnRestriction restriction;
    while (num_indices-- > 0)
    {
        read(reader, restriction);
        restrictions.push_back(std::move(restriction));
    }
}

inline void write(storage::io::FileWriter &writer,
                  const std::vector<ConditionalTurnRestriction> &restrictions)
{
    const auto num_indices = restrictions.size();
    writer.WriteElementCount64(num_indices);
    const auto write_restriction = [&writer](const auto &restriction) {
        write(writer, restriction);
    };
    std::for_each(restrictions.begin(), restrictions.end(), write_restriction);
}

inline void read(storage::io::FileReader &reader, ConditionalTurnPenalty &turn_penalty)
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

inline void write(storage::io::FileWriter &writer, const ConditionalTurnPenalty &turn_penalty)
{
    writer.WriteOne(turn_penalty.turn_offset);
    writer.WriteOne(static_cast<util::FixedLatitude::value_type>(turn_penalty.location.lat));
    writer.WriteOne(static_cast<util::FixedLongitude::value_type>(turn_penalty.location.lon));
    writer.WriteElementCount64(turn_penalty.conditions.size());
    for (const auto &c : turn_penalty.conditions)
    {
        writer.WriteOne(c.modifier);
        storage::serialization::write(writer, c.times);
        storage::serialization::write(writer, c.weekdays);
        storage::serialization::write(writer, c.monthdays);
    }
}

inline void write(storage::io::FileWriter &writer,
                  const std::vector<ConditionalTurnPenalty> &conditional_penalties)
{
    writer.WriteElementCount64(conditional_penalties.size());
    for (const auto &penalty : conditional_penalties)
        write(writer, penalty);
}

inline void read(storage::io::FileReader &reader,
                 std::vector<ConditionalTurnPenalty> &conditional_penalties)
{
    auto const num_elements = reader.ReadElementCount64();
    conditional_penalties.resize(num_elements);
    for (auto &penalty : conditional_penalties)
        read(reader, penalty);
}
}
}
}

#endif
