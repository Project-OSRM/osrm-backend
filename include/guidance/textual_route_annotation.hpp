#ifndef ENGINE_GUIDANCE_TEXTUAL_ROUTE_ANNOTATIONS_HPP_
#define ENGINE_GUIDANCE_TEXTUAL_ROUTE_ANNOTATIONS_HPP_

#include "engine/segment_information.hpp"
#include "guidance/segment_list.hpp"
#include "guidance/turn_instruction.hpp"
#include "guidance/guidance_toolkit.hpp"
#include "guidance/translator.hpp"
#include "osrm/json_container.hpp"

#include "util/bearing.hpp"
#include "util/cast.hpp"

#include "guidance/instruction_symbols.hpp"

#include <cstdint>

#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace osrm
{
namespace guidance
{
template <typename DataFacadeT>
inline util::json::Array
AnnotateRoute(const std::vector<engine::SegmentInformation> &route_segments, DataFacadeT *facade)
{
    util::json::Array json_instruction_array;
    if (route_segments.empty())
        return json_instruction_array;
    // Segment information has following format:
    //["instruction id","streetname",length,position,time,"length","earth_direction",azimuth]
    std::int32_t necessary_segments_running_index = 0;

    struct RoundAbout
    {
        std::int32_t start_index;
        NodeID name_id;
        std::uint32_t leave_at_exit;
    } round_about;

    round_about = {std::numeric_limits<std::int32_t>::max(), 0, 0};
    std::string temp_dist, temp_length, temp_duration, temp_bearing, temp_instruction;
    extractor::TravelMode last_travel_mode = TRAVEL_MODE_DEFAULT;

    // Generate annotations for every segment
    for (std::size_t i = 0; i < route_segments.size(); ++i)
    {
        const auto &segment = route_segments[i];
        util::json::Array json_instruction_row;
        TurnInstruction current_instruction = segment.turn_instruction;
        if (isTurnNecessary(current_instruction))
        {
            if (staysOnRoundabout(current_instruction))
            {
                ++round_about.leave_at_exit;
            }
            else
            {
                std::string current_turn_instruction;
                if (leavesRoundabout(current_instruction))
                {
                    temp_instruction = std::to_string(
                        util::cast::enum_to_underlying(InstructionSymbol::LeaveRoundAbout));
                    current_turn_instruction += temp_instruction;
                    current_turn_instruction += "-";
                    temp_instruction = std::to_string(round_about.leave_at_exit + 1);
                    current_turn_instruction += temp_instruction;
                }
                else
                {
                    if (current_instruction.type == TurnType::Location)
                    {
                        temp_instruction =
                            std::to_string(util::cast::enum_to_underlying(getLocationSymbol(
                                i == 0 ? LocationType::Start : LocationType::Intermediate)));
                    }
                    else
                    {
                        temp_instruction = std::to_string(
                            util::cast::enum_to_underlying(getSymbol(current_instruction)));
                    }
                    current_turn_instruction += temp_instruction;
                }
                json_instruction_row.values.emplace_back(std::move(current_turn_instruction));

                if (current_instruction.type == TurnType::Location)
                {
                    json_instruction_row.values.push_back(
                        translateLocation(i == 0 ? LocationType::Start : LocationType::Intermediate,
                                          current_instruction.direction_modifier,
                                          facade->get_name_for_id(segment.name_id)));
                } else if( leavesRoundabout(current_instruction) )
                {
                  json_instruction_row.values.push_back(translateRoundabout( LocationType::Destination, facade->get_name_for_id(segment.name_id), round_about.leave_at_exit+1 ));
                  round_about.leave_at_exit = 0;
                } else if (entersRoundabout(current_instruction))
                {
                  json_instruction_row.values.push_back(translateRoundabout( LocationType::Start, facade->get_name_for_id(segment.name_id), round_about.leave_at_exit+1 ));
                  round_about.name_id = segment.name_id;
                  round_about.start_index = necessary_segments_running_index;
                }

                else
                {
                    json_instruction_row.values.push_back(
                        translate(current_instruction, facade->get_name_for_id(segment.name_id)));
                }
                json_instruction_row.values.push_back(std::round(segment.length));
                json_instruction_row.values.push_back(necessary_segments_running_index);
                json_instruction_row.values.push_back(std::round(segment.duration / 10.));
                json_instruction_row.values.push_back(
                    std::to_string(static_cast<std::uint32_t>(segment.length)) + "m");

                // post turn bearing
                const double post_turn_bearing_value = (segment.post_turn_bearing / 10.);
                json_instruction_row.values.push_back(util::bearing::get(post_turn_bearing_value));
                json_instruction_row.values.push_back(
                    static_cast<std::uint32_t>(std::round(post_turn_bearing_value)));

                if (i + 1 < route_segments.size())
                {
                    // anounce next travel mode with turn
                    json_instruction_row.values.push_back(route_segments[i + 1].travel_mode);
                    last_travel_mode = segment.travel_mode;
                }
                else
                {
                    json_instruction_row.values.push_back(segment.travel_mode);
                    last_travel_mode = segment.travel_mode;
                }

                // pre turn bearing
                const double pre_turn_bearing_value = (segment.pre_turn_bearing / 10.);
                json_instruction_row.values.push_back(util::bearing::get(pre_turn_bearing_value));
                json_instruction_row.values.push_back(
                    static_cast<std::uint32_t>(std::round(pre_turn_bearing_value)));

                json_instruction_array.values.push_back(json_instruction_row);
            }
        }
        if (segment.necessary)
        {
            ++necessary_segments_running_index;
        }
    }

    util::json::Array json_last_instruction_row;
    temp_instruction =
        std::to_string(util::cast::enum_to_underlying(InstructionSymbol::ReachedYourDestination));
    json_last_instruction_row.values.emplace_back(std::move(temp_instruction));
    json_last_instruction_row.values.push_back(translateLocation(
        LocationType::Destination, route_segments.back().turn_instruction.direction_modifier,
        facade->get_name_for_id(route_segments.back().name_id)));
    json_last_instruction_row.values.push_back(0);
    json_last_instruction_row.values.push_back(necessary_segments_running_index - 1);
    json_last_instruction_row.values.push_back(0);
    json_last_instruction_row.values.push_back("0m");
    json_last_instruction_row.values.push_back(util::bearing::get(0.0));
    json_last_instruction_row.values.push_back(0.);
    json_last_instruction_row.values.push_back(last_travel_mode);
    json_last_instruction_row.values.push_back(util::bearing::get(0.0));
    json_last_instruction_row.values.push_back(0.);
    json_instruction_array.values.emplace_back(std::move(json_last_instruction_row));

    return json_instruction_array;
}

} // namespace guidance
} // namespace osrm

#endif
