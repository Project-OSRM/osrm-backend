#ifndef OSRM_GUIDANCE_PARSING_TOOLKIT_HPP_
#define OSRM_GUIDANCE_PARSING_TOOLKIT_HPP_

#include <cstdint>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

namespace osrm::extractor::guidance
{

// Public service vehicle lanes and similar can introduce additional lanes into the lane string that
// are not specifically marked for left/right turns. This function can be used from the profile to
// trim the lane string appropriately
//
// left|throught|
// in combination with lanes:psv:forward=1
// will be corrected to left|throught, since the final lane is not drivable.
// This is in contrast to a situation with lanes:psv:forward=0 (or not set) where left|through|
// represents left|through|through
[[nodiscard]] inline std::string
trimLaneString(std::string lane_string, std::int32_t count_left, std::int32_t count_right)
{
    if (count_left)
    {
        bool sane = count_left < static_cast<std::int32_t>(lane_string.size());
        for (std::int32_t i = 0; i < count_left; ++i)
            // this is adjusted for our fake pipe. The moment cucumber can handle multiple escaped
            // pipes, the '&' part can be removed
            if (lane_string[i] != '|')
            {
                sane = false;
                break;
            }

        if (sane)
        {
            lane_string.erase(lane_string.begin(), lane_string.begin() + count_left);
        }
    }
    if (count_right)
    {
        bool sane = count_right < static_cast<std::int32_t>(lane_string.size());
        for (auto itr = lane_string.rbegin();
             itr != lane_string.rend() && itr != lane_string.rbegin() + count_right;
             ++itr)
        {
            if (*itr != '|')
            {
                sane = false;
                break;
            }
        }
        if (sane)
            lane_string.resize(lane_string.size() - count_right);
    }
    return lane_string;
}

// https://github.com/Project-OSRM/osrm-backend/issues/2638
// It can happen that some lanes are not drivable by car. Here we handle this tagging scheme
// (vehicle:lanes) to filter out not-allowed roads
// lanes=3
// turn:lanes=left|through|through|right
// vehicle:lanes=yes|yes|no|yes
// bicycle:lanes=yes|no|designated|yes
[[nodiscard]] inline std::string applyAccessTokens(std::string lane_string,
                                                   const std::string &access_tokens)
{
    using tokenizer = boost::tokenizer<boost::char_separator<char>>;
    boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
    tokenizer tokens(lane_string, sep);
    tokenizer access(access_tokens, sep);

    // strings don't match, don't do anything
    if (std::distance(std::begin(tokens), std::end(tokens)) !=
        std::distance(std::begin(access), std::end(access)))
        return lane_string;

    std::string result_string = "";
    const static std::string yes = "yes";

    for (auto token_itr = std::begin(tokens), access_itr = std::begin(access);
         token_itr != std::end(tokens);
         ++token_itr, ++access_itr)
    {
        if (*access_itr == yes)
        {
            // we have to add this in front, because the next token could be invalid. Doing this on
            // non-empty strings makes sure that the token string will be valid in the end
            if (!result_string.empty())
                result_string += '|';

            result_string += *token_itr;
        }
    }
    return result_string;
}

} // namespace osrm::extractor::guidance

#endif // OSRM_GUIDANCE_PARSING_TOOLKIT_HPP_
