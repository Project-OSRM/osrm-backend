#ifndef OSRM_UTIL_GUIDANCE_DESTINATIONS_SIMILARITY_HPP
#define OSRM_UTIL_GUIDANCE_DESTINATIONS_SIMILARITY_HPP

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/function_output_iterator.hpp>

#include <iostream>

namespace osrm
{
namespace util
{
namespace guidance
{

template <typename T> auto getDestinationTokens(const T &destination)
{
    std::vector<std::string> splited;
    boost::split(splited, destination, boost::is_any_of(",:"));

    std::set<std::string> tokens;
    std::transform(
        splited.begin(), splited.end(), std::inserter(tokens, tokens.end()), [](auto token) {
            boost::trim(token);
            boost::to_lower(token);
            return token;
        });
    return tokens;
}

template <typename T> double getSetsSimilarity(const T &lhs, const T &rhs)
{
    if (lhs.size() + rhs.size() == 0)
        return 1.;

    std::size_t count = 0;
    auto counter = [&count](const std::string &) { ++count; };
    std::set_intersection(lhs.begin(),
                          lhs.end(),
                          rhs.begin(),
                          rhs.end(),
                          boost::make_function_output_iterator(std::ref(counter)));

    return static_cast<double>(count) / (lhs.size() + rhs.size() - count);
}

} // namespace guidance
} // namespace util
} // namespace osrm

#endif /* OSRM_UTIL_GUIDANCE_DESTINATIONS_SIMILARITY_HPP */
