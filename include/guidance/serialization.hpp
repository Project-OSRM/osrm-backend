#ifndef OSRM_GUIDANCE_IO_HPP
#define OSRM_GUIDANCE_IO_HPP

#include "guidance/turn_data_container.hpp"

#include "storage/io.hpp"
#include "storage/serialization.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace guidance
{
namespace serialization
{

// read/write for turn data file
template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 guidance::detail::TurnDataContainerImpl<Ownership> &turn_data_container,
                 std::uint32_t &connectivity_checksum)
{
    storage::serialization::read(reader, turn_data_container.turn_instructions);
    storage::serialization::read(reader, turn_data_container.lane_data_ids);
    storage::serialization::read(reader, turn_data_container.entry_class_ids);
    storage::serialization::read(reader, turn_data_container.pre_turn_bearings);
    storage::serialization::read(reader, turn_data_container.post_turn_bearings);
    reader.ReadInto(connectivity_checksum);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const guidance::detail::TurnDataContainerImpl<Ownership> &turn_data_container,
                  const std::uint32_t connectivity_checksum)
{
    storage::serialization::write(writer, turn_data_container.turn_instructions);
    storage::serialization::write(writer, turn_data_container.lane_data_ids);
    storage::serialization::write(writer, turn_data_container.entry_class_ids);
    storage::serialization::write(writer, turn_data_container.pre_turn_bearings);
    storage::serialization::write(writer, turn_data_container.post_turn_bearings);
    writer.WriteOne(connectivity_checksum);
}
}
}
}

#endif
