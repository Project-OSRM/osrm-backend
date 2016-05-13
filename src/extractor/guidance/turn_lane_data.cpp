#include "extractor/guidance/turn_lane_data.hpp"
#include "util/guidance/turn_lanes.hpp"

#include <boost/numeric/conversion/cast.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace lanes
{

bool TurnLaneData::operator<(const TurnLaneData &other) const
{
    if (from < other.from)
        return true;
    if (from > other.from)
        return false;

    if (to < other.to)
        return true;
    if (to > other.to)
        return false;

    const constexpr char *tag_by_modifier[] = {"sharp_right",
                                               "right",
                                               "slight_right",
                                               "through",
                                               "slight_left",
                                               "left",
                                               "sharp_left",
                                               "reverse"};
    return std::find(tag_by_modifier, tag_by_modifier + 8, this->tag) <
           std::find(tag_by_modifier, tag_by_modifier + 8, other.tag);
}

LaneDataVector laneDataFromString(std::string turn_lane_string)
{
    typedef std::unordered_map<std::string, std::pair<LaneID, LaneID>> LaneMap;

    // FIXME this is a workaround due to https://github.com/cucumber/cucumber-js/issues/417,
    // need to switch statements when fixed
    // const auto num_lanes = std::count(turn_lane_string.begin(), turn_lane_string.end(), '|') + 1;
    // count the number of lanes
    const auto num_lanes = [](const std::string &turn_lane_string) {
        return boost::numeric_cast<LaneID>(
            std::count(turn_lane_string.begin(), turn_lane_string.end(), '|') + 1 +
            std::count(turn_lane_string.begin(), turn_lane_string.end(), '&'));
    }(turn_lane_string);

    const auto getNextTag = [](std::string &string, const char *separators) {
        auto pos = string.find_last_of(separators);
        auto result = pos != std::string::npos ? string.substr(pos + 1) : string;

        string.resize(pos == std::string::npos ? 0 : pos);
        return result;
    };

    const auto setLaneData = [&](LaneMap &map, std::string lane, const LaneID current_lane) {
        do
        {
            auto identifier = getNextTag(lane, ";");
            if (identifier.empty())
                identifier = "none";
            auto map_iterator = map.find(identifier);
            if (map_iterator == map.end())
                map[identifier] = std::make_pair(current_lane, current_lane);
            else
            {
                map_iterator->second.second = current_lane;
            }
        } while (!lane.empty());
    };

    // check whether a given turn lane string resulted in valid lane data
    const auto hasValidOverlaps = [](const LaneDataVector &lane_data) {
        // Allow an overlap of at most one. Larger overlaps would result in crossing another turn,
        // which is invalid
        for (std::size_t index = 1; index < lane_data.size(); ++index)
        {
            if (lane_data[index - 1].to > lane_data[index].from)
                return false;
        }
        return true;
    };

    LaneMap lane_map;
    LaneID lane_nr = 0;
    LaneDataVector lane_data;
    if (turn_lane_string.empty())
        return lane_data;

    do
    {
        // FIXME this is a cucumber workaround, since escaping does not work properly in
        // cucumber.js (see https://github.com/cucumber/cucumber-js/issues/417). Needs to be
        // changed to "|" only, when the bug is fixed
        auto lane = getNextTag(turn_lane_string, "|&");
        setLaneData(lane_map, lane, lane_nr);
        ++lane_nr;
    } while (lane_nr < num_lanes);

    for (const auto tag : lane_map)
    {
        lane_data.push_back({tag.first, tag.second.first, tag.second.second});
    }

    std::sort(lane_data.begin(), lane_data.end());
    if (!hasValidOverlaps(lane_data))
    {
        lane_data.clear();
    }

    return lane_data;
}

LaneDataVector::iterator findTag(const std::string &tag, LaneDataVector &data)
{
    return std::find_if(data.begin(), data.end(), [&](const TurnLaneData &lane_data) {
        return tag == lane_data.tag;
    });
}
LaneDataVector::const_iterator findTag(const std::string &tag, const LaneDataVector &data)
{
    return std::find_if(data.cbegin(), data.cend(), [&](const TurnLaneData &lane_data) {
        return tag == lane_data.tag;
    });
}

bool hasTag(const std::string &tag, const LaneDataVector &data){
    return findTag(tag,data) != data.cend();
}

} // namespace lanes
} // namespace guidance
} // namespace extractor
} // namespace osrm
