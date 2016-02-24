#include "engine/guidance/post_processing.hpp"
#include "engine/guidance/turn_instruction.hpp"
#include "engine/guidance/guidance_toolkit.hpp"

#include <boost/assert.hpp>
#include <iostream>

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
           destination.travel_mode == source.travel_mode && isSilent(destination.turn_instruction);
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
    if (source.turn_instruction == TurnType::Suppressed &&
        detail::canMergeTrivially(destination, source))
    {
        return detail::accumulateInto(destination, source);
    }
    if (source.turn_instruction.type == TurnType::StayOnRoundabout)
    {
        return detail::accumulateInto(destination, source);
    }
    return destination;
}

} // namespace detail

void print( const std::vector<std::vector<PathData>> & leg_data )
{
  std::cout << "Path\n";
  int legnr = 0;
  for( const auto & leg : leg_data )
  {
    std::cout << "\tLeg: " << ++legnr << "\n";
    int segment = 0;
    for( const auto &data : leg ){
      std::cout << "\t\t[" << ++segment << "]: " << (int) data.turn_instruction.type << " " << (int)data.turn_instruction.direction_modifier << " exit: " << data.exit << "\n";
    }
  }
  std::cout << std::endl;
}

std::vector<std::vector<PathData>> postProcess(std::vector<std::vector<PathData>> leg_data)
{
    std::cout << "[POSTPROCESSING ITERATION]" << std::endl;
    unsigned carry_exit = 0;
    // Count Street Exits forward
    print( leg_data );
    for (auto &path_data : leg_data)
    {
        path_data[0].exit = carry_exit;
        for (std::size_t data_index = 0; data_index + 1 < path_data.size(); ++data_index)
        {
            if (path_data[data_index].turn_instruction.type == TurnType::EnterRoundaboutAtExit)
            {
                path_data[data_index].exit += 1; // Count the exit
                path_data[data_index].turn_instruction.type = TurnType::EnterRoundabout;
            }
            else if (path_data[data_index].turn_instruction.type == TurnType::EnterRotaryAtExit)
            {
                path_data[data_index].exit += 1;
                path_data[data_index].turn_instruction.type = TurnType::EnterRotary;
            }

            if (isSilent(path_data[data_index].turn_instruction))
            {
                path_data[data_index + 1] =
                    detail::mergeInto(path_data[data_index + 1], path_data[data_index]);
            }
            carry_exit = path_data[data_index].exit;
        }
    }

    print( leg_data );
    // Move Roundabout exit numbers to front
    for (auto rev_itr = leg_data.rbegin(); rev_itr != leg_data.rend(); ++rev_itr)
    {
        auto &path_data = *rev_itr;
        for (std::size_t data_index = path_data.size(); data_index > 1; --data_index)
        {
            if (leavesRoundabout(path_data[data_index - 1].turn_instruction) ||
                staysOnRoundabout(path_data[data_index - 1].turn_instruction))
            {
                path_data[data_index - 2].exit = path_data[data_index - 1].exit;
            }
        }
        auto prev_leg = std::next(rev_itr);
        if (!path_data.empty() && prev_leg != leg_data.rend())
        {
            if (staysOnRoundabout(path_data[0].turn_instruction) ||
                leavesRoundabout(path_data[0].turn_instruction))
            {
                prev_leg->back().exit = path_data[0].exit;
            }
        }
    }

    print( leg_data );
    // silence turns for good
    for (auto &path_data : leg_data)
    {
        for (auto &data : path_data)
        {
            if (isSilent(data.turn_instruction) || leavesRoundabout(data.turn_instruction))
                data.turn_instruction = TurnInstruction::NO_TURN();
        }
    }

    print( leg_data );
    return std::move(leg_data);
}

} // namespace guidance
} // namespace engine
} // namespace osrm
