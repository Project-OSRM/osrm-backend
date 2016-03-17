#include "engine/guidance/post_processing.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include "engine/guidance/toolkit.hpp"

#include <boost/assert.hpp>
#include <iostream>
#include <vector>

using TurnInstruction = osrm::extractor::guidance::TurnInstruction;
using TurnType = osrm::extractor::guidance::TurnType;
using DirectionModifier = osrm::extractor::guidance::DirectionModifier;

namespace osrm
{
namespace engine
{
namespace guidance
{

namespace detail
{
bool canMergeTrivially(const PathData &destination, const PathData &source)
{
    return destination.exit == 0 && destination.name_id == source.name_id &&
           destination.travel_mode == source.travel_mode && isSilent(source.turn_instruction);
}

PathData forwardInto(PathData destination, const PathData &source)
{
    // Merge a turn into a silent turn
    // Overwrites turn instruction and increases exit NR
    destination.duration_until_turn += source.duration_until_turn;
    destination.exit = source.exit;
    return destination;
}

PathData accumulateInto(PathData destination, const PathData &source)
{
    // Merge a turn into a silent turn
    // Overwrites turn instruction and increases exit NR
    BOOST_ASSERT(canMergeTrivially(destination, source));
    destination.duration_until_turn += source.duration_until_turn;
    destination.exit = source.exit + 1;
    return destination;
}

PathData mergeInto(PathData destination, const PathData &source)
{
    if (source.turn_instruction == TurnInstruction::NO_TURN())
    {
        BOOST_ASSERT(canMergeTrivially(destination, source));
        return detail::forwardInto(destination, source);
    }
    if (source.turn_instruction.type == TurnType::Suppressed)
    {
        return detail::forwardInto(destination, source);
    }
    if (source.turn_instruction.type == TurnType::StayOnRoundabout)
    {
        return detail::forwardInto(destination, source);
    }
    if (entersRoundabout(source.turn_instruction))
    {
        return detail::forwardInto(destination, source);
    }
    return destination;
}

} // namespace detail

void print(const std::vector<std::vector<PathData>> &leg_data)
{
    std::cout << "Path\n";
    int legnr = 0;
    for (const auto &leg : leg_data)
    {
        std::cout << "\tLeg: " << ++legnr << "\n";
        int segment = 0;
        for (const auto &data : leg)
        {
            const auto type = static_cast<int>(data.turn_instruction.type);
            const auto modifier = static_cast<int>(data.turn_instruction.direction_modifier);

            std::cout << "\t\t[" << ++segment << "]: " << type << " " << modifier
                      << " exit: " << data.exit << "\n";
        }
    }
    std::cout << std::endl;
}

std::vector<std::vector<PathData>> postProcess(std::vector<std::vector<PathData>> leg_data)
{
    if (leg_data.empty())
        return leg_data;

#define PRINT_DEBUG 0
    unsigned carry_exit = 0;
#if PRINT_DEBUG
    std::cout << "[POSTPROCESSING ITERATION]" << std::endl;
    std::cout << "Input\n";
    print(leg_data);
#endif
    // Count Street Exits forward
    bool on_roundabout = false;
    for (auto &path_data : leg_data)
    {
        if (not path_data.empty())
            path_data[0].exit = carry_exit;

        for (std::size_t data_index = 0; data_index + 1 < path_data.size(); ++data_index)
        {
            if (entersRoundabout(path_data[data_index].turn_instruction))
            {
                path_data[data_index].exit += 1;
                on_roundabout = true;
            }

            if (isSilent(path_data[data_index].turn_instruction) &&
                path_data[data_index].turn_instruction != TurnInstruction::NO_TURN())
            {
                path_data[data_index].exit += 1;
            }
            if (leavesRoundabout(path_data[data_index].turn_instruction))
            {
                if (!on_roundabout)
                {
                    BOOST_ASSERT(leg_data[0][0].turn_instruction.type ==
                                 TurnInstruction::NO_TURN());
                    if (path_data[data_index].turn_instruction.type == TurnType::ExitRoundabout)
                        leg_data[0][0].turn_instruction.type = TurnType::EnterRoundabout;
                    if (path_data[data_index].turn_instruction.type == TurnType::ExitRotary)
                        leg_data[0][0].turn_instruction.type = TurnType::EnterRotary;
                    path_data[data_index].exit += 1;
                }
                on_roundabout = false;
            }
            if (path_data[data_index].turn_instruction.type == TurnType::EnterRoundaboutAtExit)
            {
                path_data[data_index].exit += 1;
                path_data[data_index].turn_instruction.type = TurnType::EnterRoundabout;
            }
            else if (path_data[data_index].turn_instruction.type == TurnType::EnterRotaryAtExit)
            {
                path_data[data_index].exit += 1;
                path_data[data_index].turn_instruction.type = TurnType::EnterRotary;
            }

            if (isSilent(path_data[data_index].turn_instruction) ||
                entersRoundabout(path_data[data_index].turn_instruction))
            {
                path_data[data_index + 1] =
                    detail::mergeInto(path_data[data_index + 1], path_data[data_index]);
            }
            carry_exit = path_data[data_index].exit;
        }
    }
#if PRINT_DEBUG
    std::cout << "Merged\n";
    print(leg_data);
#endif
    on_roundabout = false;
    // Move Roundabout exit numbers to front
    for (auto rev_itr = leg_data.rbegin(); rev_itr != leg_data.rend(); ++rev_itr)
    {
        auto &path_data = *rev_itr;
        for (std::size_t data_index = path_data.size(); data_index > 1; --data_index)
        {
            if (entersRoundabout(path_data[data_index - 1].turn_instruction))
            {
                if (!on_roundabout && !leavesRoundabout(path_data[data_index - 1].turn_instruction))
                    path_data[data_index - 1].exit = 0;
                on_roundabout = false;
            }
            if (on_roundabout)
            {
                path_data[data_index - 2].exit = path_data[data_index - 1].exit;
            }
            if (leavesRoundabout(path_data[data_index - 1].turn_instruction) &&
                !entersRoundabout(path_data[data_index - 1].turn_instruction))
            {
                path_data[data_index - 2].exit = path_data[data_index - 1].exit;
                on_roundabout = true;
            }
        }
        auto prev_leg = std::next(rev_itr);
        if (!path_data.empty() && prev_leg != leg_data.rend())
        {
            if (on_roundabout && path_data[0].exit)
                prev_leg->back().exit = path_data[0].exit;
        }
    }

#if PRINT_DEBUG
    std::cout << "Move To Front\n";
    print(leg_data);
#endif
    // silence silent turns for good
    for (auto &path_data : leg_data)
    {
        for (auto &data : path_data)
        {
            if (isSilent(data.turn_instruction) || (leavesRoundabout(data.turn_instruction) &&
                                                    !entersRoundabout(data.turn_instruction)))
            {
                data.turn_instruction = TurnInstruction::NO_TURN();
                data.exit = 0;
            }
        }
    }

    return leg_data;
}

} // namespace guidance
} // namespace engine
} // namespace osrm
