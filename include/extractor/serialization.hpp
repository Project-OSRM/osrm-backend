#ifndef OSRM_EXTRACTOR_IO_HPP
#define OSRM_EXTRACTOR_IO_HPP

#include "extractor/conditional_turn_penalty.hpp"
#include "extractor/datasources.hpp"
#include "extractor/intersection_bearings_container.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/nbg_to_ebg.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/restriction.hpp"
#include "extractor/segment_cost_encoding.hpp"
#include "extractor/segment_data_container.hpp"
#include "extractor/string_table.hpp"

#include "storage/io.hpp"
#include "storage/serialization.hpp"

namespace osrm::extractor::serialization
{

// read/write for bearing data
template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::IntersectionBearingsContainer<Ownership> &intersection_bearings)
{
    storage::serialization::read(reader, name + "/bearing_values", intersection_bearings.values_);
    storage::serialization::read(
        reader, name + "/node_to_class_id", intersection_bearings.node_to_class_id_);
    util::serialization::read(
        reader, name + "/class_id_to_ranges", intersection_bearings.class_id_to_ranges_table_);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::IntersectionBearingsContainer<Ownership> &intersection_bearings)
{
    storage::serialization::write(writer, name + "/bearing_values", intersection_bearings.values_);
    storage::serialization::write(
        writer, name + "/node_to_class_id", intersection_bearings.node_to_class_id_);
    util::serialization::write(
        writer, name + "/class_id_to_ranges", intersection_bearings.class_id_to_ranges_table_);
}

// read/write for properties file
inline void
read(storage::tar::FileReader &reader, const std::string &name, ProfileProperties &properties)
{ reader.ReadInto(name, properties); }

inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const ProfileProperties &properties)
{
    writer.WriteElementCount64(name, 1);
    writer.WriteFrom(name, properties);
}

// read/write for datasources file
inline void read(storage::tar::FileReader &reader, const std::string &name, Datasources &sources)
{ reader.ReadInto(name, sources); }

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
    const auto cost_prefix = name + "/adaptive_costs";
    const auto forward_sources_name = reader.HasEntry(name + "/constant_forward_sources")
                                          ? name + "/constant_forward_sources"
                                          : name + "/forward_data_sources";
    const auto reverse_sources_name = reader.HasEntry(name + "/constant_reverse_sources")
                                          ? name + "/constant_reverse_sources"
                                          : name + "/reverse_data_sources";
    storage::serialization::read(reader, name + "/index", segment_data.index);
    storage::serialization::read(reader, name + "/nodes", segment_data.nodes);
    if (reader.HasEntry(cost_prefix + "/descriptors"))
    {
        {
            storage::serialization::read(
                reader, cost_prefix + "/descriptors", segment_data.shared_descriptors);
            util::serialization::read(reader, cost_prefix, segment_data.shared_weights);
            util::serialization::read(reader, cost_prefix, segment_data.shared_durations);
            storage::serialization::read(
                reader, forward_sources_name, segment_data.fwd_datasources);
            storage::serialization::read(
                reader, reverse_sources_name, segment_data.rev_datasources);
            if constexpr (Ownership != storage::Ownership::View)
            {
                segment_data.fwd_weights.resize(segment_data.nodes.size());
                segment_data.rev_weights.resize(segment_data.nodes.size());
                segment_data.fwd_durations.resize(segment_data.nodes.size());
                segment_data.rev_durations.resize(segment_data.nodes.size());
                const auto &source = std::as_const(segment_data);
                for (std::size_t g = 0; g < segment_data.GetNumberOfGeometries(); ++g)
                {
                    const auto begin = segment_data.index[g], end = segment_data.index[g + 1];
                    auto fw = source.GetForwardWeights(g);
                    auto rw = source.GetReverseWeights(g);
                    auto fd = source.GetForwardDurations(g);
                    auto rd = source.GetReverseDurations(g);
                    for (std::size_t k = 0; k + 1 < end - begin; ++k)
                    {
                        segment_data.fwd_weights[begin + 1 + k] = fw[k];
                        segment_data.rev_weights[end - 2 - k] = rw[k];
                        segment_data.fwd_durations[begin + 1 + k] = fd[k];
                        segment_data.rev_durations[end - 2 - k] = rd[k];
                    }
                }
                segment_data.shared_descriptors.clear();
                segment_data.shared_weights = {};
                segment_data.shared_durations = {};
                if (segment_data.fwd_datasources.size() != segment_data.nodes.size())
                    segment_data.fwd_datasources.assign(segment_data.nodes.size(), 0);
                if (segment_data.rev_datasources.size() != segment_data.nodes.size())
                    segment_data.rev_datasources.assign(segment_data.nodes.size(), 0);
            }
            return;
        }
    }
    util::serialization::read(reader, name + "/forward_weights", segment_data.fwd_weights);
    util::serialization::read(reader, name + "/reverse_weights", segment_data.rev_weights);
    util::serialization::read(reader, name + "/forward_durations", segment_data.fwd_durations);
    util::serialization::read(reader, name + "/reverse_durations", segment_data.rev_durations);
    storage::serialization::read(reader, forward_sources_name, segment_data.fwd_datasources);
    storage::serialization::read(reader, reverse_sources_name, segment_data.rev_datasources);
    if constexpr (Ownership != storage::Ownership::View)
    {
        if (segment_data.fwd_datasources.size() != segment_data.nodes.size())
            segment_data.fwd_datasources.assign(segment_data.nodes.size(), 0);
        if (segment_data.rev_datasources.size() != segment_data.nodes.size())
            segment_data.rev_datasources.assign(segment_data.nodes.size(), 0);
    }
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::SegmentDataContainerImpl<Ownership> &segment_data)
{
    storage::serialization::write(writer, name + "/index", segment_data.index);
    storage::serialization::write(writer, name + "/nodes", segment_data.nodes);
    if (!segment_data.shared_descriptors.empty())
    {
        const auto prefix = name + "/adaptive_costs";
        storage::serialization::write(
            writer, prefix + "/descriptors", segment_data.shared_descriptors);
        util::serialization::write(writer, prefix, segment_data.shared_weights);
    }
    else
    {
        auto costs = storage::encodeGeometryCosts(segment_data);
        const auto cost_bytes = ((segment_data.nodes.size() * SEGMENT_WEIGHT_BITS + 63) / 64 + 1) *
                                sizeof(std::uint64_t) * 4;
        if (costs &&
            (costs->Descriptors.size() + costs->Words.size()) * sizeof(std::uint64_t) < cost_bytes)
        {
            const auto prefix = name + "/adaptive_costs";
            storage::serialization::write(writer, prefix + "/descriptors", costs->Descriptors);
            storage::serialization::write(writer, prefix + "/packed", costs->Words);
            writer.WriteFrom(prefix + "/number_of_elements.meta", costs->Entries);
        }
        else
        {
            util::serialization::write(writer, name + "/forward_weights", segment_data.fwd_weights);
            util::serialization::write(writer, name + "/reverse_weights", segment_data.rev_weights);
            util::serialization::write(
                writer, name + "/forward_durations", segment_data.fwd_durations);
            util::serialization::write(
                writer, name + "/reverse_durations", segment_data.rev_durations);
        }
    }
    std::size_t max_length = 0;
    for (std::size_t g = 0; g < segment_data.GetNumberOfGeometries(); ++g)
        max_length = std::max(max_length,
                              std::size_t(segment_data.index[g + 1] - segment_data.index[g] - 1));
    const auto sources = [&](const auto &values, const char *role)
    {
        if (max_length < values.size() &&
            std::all_of(values.begin(), values.end(), [](auto value) { return value == 0; }))
            storage::serialization::write(
                writer, name + "/constant_" + role, std::vector<DatasourceID>(max_length, 0));
        else
            storage::serialization::write(writer,
                                          name + (std::string(role) == "forward_sources"
                                                      ? "/forward_data_sources"
                                                      : "/reverse_data_sources"),
                                          values);
    };
    sources(segment_data.fwd_datasources, "forward_sources");
    sources(segment_data.rev_datasources, "reverse_sources");
}

template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::EdgeBasedNodeDataContainerImpl<Ownership> &node_data_container)
{
    // read actual data
    if (reader.HasEntry(name + "/nodes8"))
    {
        storage::serialization::read(reader, name + "/nodes8", node_data_container.packed_nodes);
        if constexpr (Ownership != storage::Ownership::View)
        {
            node_data_container.nodes.resize(node_data_container.packed_nodes.size());
            for (std::size_t i = 0; i < node_data_container.nodes.size(); ++i)
                node_data_container.nodes[i] = node_data_container.GetNode(i);
            node_data_container.packed_nodes.clear();
        }
    }
    else
        storage::serialization::read(reader, name + "/nodes", node_data_container.nodes);
    storage::serialization::read(
        reader, name + "/annotations", node_data_container.annotation_data);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::EdgeBasedNodeDataContainerImpl<Ownership> &node_data_container)
{
    const auto count = node_data_container.NumberOfNodes();
    bool narrow = true;
    for (std::size_t i = 0; i < count; ++i)
    {
        const auto node = node_data_container.GetNode(i);
        narrow &= node.geometry_id.id <= 0xffffff && node.component_id.id <= 0x1ffff &&
                  node.annotation_id <= 0xfffff;
    }
    if (narrow)
    {
        std::vector<std::uint64_t> packed(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto node = node_data_container.GetNode(i);
            packed[i] = std::uint64_t(node.geometry_id.id) |
                        (std::uint64_t(node.geometry_id.forward) << 24) |
                        (std::uint64_t(node.component_id.id) << 25) |
                        (std::uint64_t(node.component_id.is_tiny) << 42) |
                        (std::uint64_t(node.annotation_id) << 43) |
                        (std::uint64_t(node.segregated) << 63);
        }
        storage::serialization::write(writer, name + "/nodes8", packed);
    }
    else
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
                  const detail::StringTableImpl<Ownership> &string_table)
{
    storage::io::BufferWriter buffer_writer;
    util::serialization::write(writer, name, string_table.indexed_data);
}

template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::StringTableImpl<Ownership> &string_table)
{ util::serialization::read(reader, name, string_table.indexed_data); }
} // namespace osrm::extractor::serialization

#endif
