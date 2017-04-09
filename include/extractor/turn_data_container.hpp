#ifndef OSRM_EXTRACTOR_TURN_DATA_CONTAINER_HPP
#define OSRM_EXTRACTOR_TURN_DATA_CONTAINER_HPP

#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"

#include "storage/io_fwd.hpp"
#include "storage/shared_memory_ownership.hpp"

#include "util/guidance/turn_bearing.hpp"
#include "util/vector_view.hpp"

#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{
namespace detail
{
template <storage::Ownership Ownership> class TurnDataContainerImpl;
}

namespace serialization
{
template <storage::Ownership Ownership>
void read(storage::io::FileReader &reader, detail::TurnDataContainerImpl<Ownership> &turn_data);

template <storage::Ownership Ownership>
void write(storage::io::FileWriter &writer,
           const detail::TurnDataContainerImpl<Ownership> &turn_data);
}

namespace detail
{
template <storage::Ownership Ownership> class TurnDataContainerImpl
{
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;

  public:
    TurnDataContainerImpl() = default;

    TurnDataContainerImpl(Vector<GeometryID> geometry_ids_,
                          Vector<NameID> name_ids_,
                          Vector<extractor::guidance::TurnInstruction> turn_instructions_,
                          Vector<LaneDataID> lane_data_ids_,
                          Vector<extractor::TravelMode> travel_modes_,
                          Vector<EntryClassID> entry_class_ids_,
                          Vector<util::guidance::TurnBearing> pre_turn_bearings_,
                          Vector<util::guidance::TurnBearing> post_turn_bearings_)
        : geometry_ids(std::move(geometry_ids_)), name_ids(std::move(name_ids_)),
          turn_instructions(std::move(turn_instructions_)),
          lane_data_ids(std::move(lane_data_ids_)), travel_modes(std::move(travel_modes_)),
          entry_class_ids(std::move(entry_class_ids_)),
          pre_turn_bearings(std::move(pre_turn_bearings_)),
          post_turn_bearings(std::move(post_turn_bearings_))
    {
    }

    GeometryID GetGeometryID(const EdgeID id) const { return geometry_ids[id]; }

    EntryClassID GetEntryClassID(const EdgeID id) const { return entry_class_ids[id]; }

    extractor::TravelMode GetTravelMode(const EdgeID id) const { return travel_modes[id]; }

    util::guidance::TurnBearing GetPreTurnBearing(const EdgeID id) const
    {
        return pre_turn_bearings[id];
    }

    util::guidance::TurnBearing GetPostTurnBearing(const EdgeID id) const
    {
        return post_turn_bearings[id];
    }

    LaneDataID GetLaneDataID(const EdgeID id) const { return lane_data_ids[id]; }

    bool HasLaneData(const EdgeID id) const { return INVALID_LANE_DATAID != lane_data_ids[id]; }

    NameID GetNameID(const EdgeID id) const { return name_ids[id]; }

    extractor::guidance::TurnInstruction GetTurnInstruction(const EdgeID id) const
    {
        return turn_instructions[id];
    }

    // Used by EdgeBasedGraphFactory to fill data structure
    template <typename = std::enable_if<Ownership == storage::Ownership::Container>>
    void push_back(GeometryID geometry_id,
                   NameID name_id,
                   extractor::guidance::TurnInstruction turn_instruction,
                   LaneDataID lane_data_id,
                   EntryClassID entry_class_id,
                   extractor::TravelMode travel_mode,
                   util::guidance::TurnBearing pre_turn_bearing,
                   util::guidance::TurnBearing post_turn_bearing)
    {
        geometry_ids.push_back(geometry_id);
        name_ids.push_back(name_id);
        turn_instructions.push_back(turn_instruction);
        lane_data_ids.push_back(lane_data_id);
        travel_modes.push_back(travel_mode);
        entry_class_ids.push_back(entry_class_id);
        pre_turn_bearings.push_back(pre_turn_bearing);
        post_turn_bearings.push_back(post_turn_bearing);
    }

    friend void serialization::read<Ownership>(storage::io::FileReader &reader,
                                               TurnDataContainerImpl &turn_data_container);
    friend void serialization::write<Ownership>(storage::io::FileWriter &writer,
                                                const TurnDataContainerImpl &turn_data_container);

  private:
    Vector<GeometryID> geometry_ids;
    Vector<NameID> name_ids;
    Vector<extractor::guidance::TurnInstruction> turn_instructions;
    Vector<LaneDataID> lane_data_ids;
    Vector<extractor::TravelMode> travel_modes;
    Vector<EntryClassID> entry_class_ids;
    Vector<util::guidance::TurnBearing> pre_turn_bearings;
    Vector<util::guidance::TurnBearing> post_turn_bearings;
};
}

using TurnDataExternalContainer = detail::TurnDataContainerImpl<storage::Ownership::External>;
using TurnDataContainer = detail::TurnDataContainerImpl<storage::Ownership::Container>;
using TurnDataView = detail::TurnDataContainerImpl<storage::Ownership::View>;
}
}

#endif
