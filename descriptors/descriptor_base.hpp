/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef DESCRIPTOR_BASE_HPP
#define DESCRIPTOR_BASE_HPP

#include "description_factory.hpp"
#include "../algorithms/route_name_extraction.hpp"
#include "../data_structures/coordinate_calculation.hpp"
#include "../data_structures/internal_route_result.hpp"
#include "../data_structures/phantom_node.hpp"
#include "../Util/bearing.hpp"
#include "../Util/cast.hpp"
#include "../typedefs.h"

#include <osrm/json_container.hpp>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

struct DescriptorTable : public std::unordered_map<std::string, unsigned>
{
    using super_class = typename std::unordered_map<std::string, unsigned>;
    DescriptorTable(super_class &&map)
    {
        super_class(std::forward<super_class>(map));
    }
    unsigned get_id(const std::string &key) const
    {
        const auto iter = find(key);
        if (iter != end())
        {
            return iter->second;
        }
        return 0;
    }
};

struct DescriptorConfig
{
    DescriptorConfig() : instructions(true), geometry(true), encode_geometry(true), zoom_level(18)
    {
    }

    template <class OtherT>
    DescriptorConfig(const OtherT &other)
        : instructions(other.print_instructions), geometry(other.geometry),
          encode_geometry(other.compression), zoom_level(other.zoom_level)
    {
        BOOST_ASSERT(zoom_level >= 0);
    }

    bool instructions;
    bool geometry;
    bool encode_geometry;
    short zoom_level;
};

template <class DataFacadeT> class BaseDescriptor
{
public:
    DataFacadeT *facade;
    DescriptorConfig config;
    DescriptionFactory description_factory, alternate_description_factory;
    FixedPointCoordinate current;
    unsigned entered_restricted_area_count;
    struct RoundAbout
    {
        RoundAbout() : start_index(INT_MAX), name_id(INVALID_NAMEID), leave_at_exit(INT_MAX) {}
        int start_index;
        unsigned name_id;
        int leave_at_exit;
    } round_about;

    struct Segment
    {
        Segment() : name_id(INVALID_NAMEID), length(-1), position(0) {}
        Segment(unsigned n, int l, unsigned p) : name_id(n), length(l), position(p) {}
        unsigned name_id;
        int length;
        unsigned position;
    };
    std::vector<Segment> shortest_path_segments, alternative_path_segments;
    ExtractRouteNames<DataFacadeT, Segment> GenerateRouteNames;

    BaseDescriptor(DataFacadeT *facade) : facade(facade), entered_restricted_area_count(0) {}

    void SetConfig(const DescriptorConfig &c) { config = c; }

    unsigned DescribeLeg(const std::vector<PathData> &route_leg,
                         const PhantomNodes &leg_phantoms,
                         const bool target_traversed_in_reverse,
                         const bool is_via_leg)
    {
        unsigned added_element_count = 0;
        // Get all the coordinates for the computed route
        FixedPointCoordinate current_coordinate;
        for (const PathData &path_data : route_leg)
        {
            current_coordinate = facade->GetCoordinateOfNode(path_data.node);
            description_factory.AppendSegment(current_coordinate, path_data);
            ++added_element_count;
        }
        description_factory.SetEndSegment(leg_phantoms.target_phantom, target_traversed_in_reverse, is_via_leg);
        ++added_element_count;
        BOOST_ASSERT((route_leg.size() + 1) == added_element_count);
        return added_element_count;
    }

    struct Instruction
    {
        Instruction() : instruction_id(""), street_name(""), length(0),
            position(0), time(0), length_string(""), bearing(""), azimuth(0),
            travel_mode(0)
            {}
        Instruction(const std::string &i, const std::string &s, const int l,
            const int p, const int t, const std::string &ls, const std::string &e,
            const int a, TravelMode const tm) : instruction_id(i), street_name(s), length(l),
            position(p), time(t), length_string(ls), bearing(e), azimuth(a), travel_mode(tm)
            {}
        std::string instruction_id;
        std::string street_name;
        int length;
        int position;
        int time;
        std::string length_string;
        std::string bearing;
        int azimuth;
        TravelMode travel_mode;
    };



    inline void BuildTextualDescription(DescriptionFactory &description_factory,
                                        std::vector<Instruction> &instructions,
                                        const int route_length,
                                        std::vector<Segment> &route_segments_list)
    {
        // Segment information has following format:
        //["instruction id","streetname",length,position,time,"length","earth_direction",azimuth]
        unsigned necessary_segments_running_index = 0;
        round_about.leave_at_exit = 0;
        round_about.name_id = 0;
        std::string temp_dist, temp_length, temp_duration, temp_bearing, temp_instruction;

        // Fetch data from Factory and generate a string from it.
        Instruction instruction;

        for (const SegmentInformation &segment : description_factory.path_description)
        {
            TurnInstruction current_instruction = segment.turn_instruction;
            entered_restricted_area_count += (current_instruction != segment.turn_instruction);
            if (TurnInstructionsClass::TurnIsNecessary(current_instruction))
            {
                if (TurnInstruction::EnterRoundAbout == current_instruction)
                {
                    round_about.name_id = segment.name_id;
                    round_about.start_index = necessary_segments_running_index;
                }
                else
                {
                    std::string current_turn_instruction;
                    if (TurnInstruction::LeaveRoundAbout == current_instruction)
                    {
                        temp_instruction =
                            cast::integral_to_string(cast::enum_to_underlying(TurnInstruction::EnterRoundAbout));
                        current_turn_instruction += temp_instruction;
                        current_turn_instruction += "-";
                        temp_instruction = cast::integral_to_string(round_about.leave_at_exit + 1);
                        current_turn_instruction += temp_instruction;
                        round_about.leave_at_exit = 0;
                    }
                    else
                    {
                        temp_instruction = cast::integral_to_string(cast::enum_to_underlying(current_instruction));
                        current_turn_instruction += temp_instruction;
                    }

                    instruction.instruction_id = current_turn_instruction;
                    instruction.street_name = facade->GetEscapedNameForNameID(segment.name_id);
                    instruction.length = std::round(segment.length);
                    instruction.position = necessary_segments_running_index;
                    instruction.time = round(segment.duration / 10);
                    instruction.length_string = cast::integral_to_string(static_cast<int>(segment.length)) + "m";
                    const double bearing_value = (segment.bearing / 10.);
                    instruction.bearing = Bearing::Get(bearing_value);
                    instruction.azimuth = static_cast<unsigned>(round(bearing_value));
                    instruction.travel_mode = segment.travel_mode;

                    route_segments_list.emplace_back(
                        segment.name_id, static_cast<int>(segment.length), static_cast<unsigned>(route_segments_list.size()));
                    instructions.push_back(instruction);
                }
            }
            else if (TurnInstruction::StayOnRoundAbout == current_instruction)
            {
                ++round_about.leave_at_exit;
            }
            if (segment.necessary)
            {
                ++necessary_segments_running_index;
            }
        }

        temp_instruction = cast::integral_to_string(cast::enum_to_underlying(TurnInstruction::ReachedYourDestination));
        instruction.instruction_id = temp_instruction;
        instruction.street_name = "";
        instruction.length = 0;
        instruction.position = necessary_segments_running_index - 1;
        instruction.time = 0;
        instruction.length_string = "0m";
        instruction.bearing = Bearing::Get(0.0);
        instruction.azimuth = 0.;
        instructions.push_back(instruction);
    }

    virtual ~BaseDescriptor() {}
    virtual void Run(const InternalRouteResult &raw_route, osrm::json::Object &json_result) = 0;
};

#endif // DESCRIPTOR_BASE_HPP
