#ifndef ENGINE_API_TABLE_PARAMETERS_HPP
#define ENGINE_API_TABLE_PARAMETERS_HPP

#include "engine/api/base_parameters.hpp"

#include <cstddef>

#include <algorithm>
#include <iterator>
#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

struct TableParameters : public BaseParameters
{
    std::vector<std::size_t> sources;
    std::vector<std::size_t> destinations;

    TableParameters() = default;
    template <typename... Args>
    TableParameters(std::vector<std::size_t> sources_,
                    std::vector<std::size_t> destinations_,
                    Args... args_)
        : BaseParameters{std::forward<Args>(args_)...}, sources{std::move(sources_)},
          destinations{std::move(destinations_)}
    {
    }

    bool IsValid() const
    {
        if (!BaseParameters::IsValid())
            return false;

        // Distance Table makes only sense with 2+ coordinates and 1+ sources and 1+ destinations
        if (coordinates.size() < 2 || sources.size() < 1 || destinations.size() < 1)
            return false;

        // 1/ The user is able to specify duplicates in srcs and dsts, in that case it's her fault

        // 2/ len(srcs) and len(dsts) smaller or equal to len(locations)
        if (sources.size() > coordinates.size())
            return false;

        if (destinations.size() > coordinates.size())
            return false;

        // 3/ 0 <= index < len(locations)
        const auto not_in_range = [this](const std::size_t x)
        {
            return x >= coordinates.size();
        };

        if (std::any_of(begin(sources), end(sources), not_in_range))
            return false;

        if (std::any_of(begin(destinations), end(destinations), not_in_range))
            return false;

        return true;
    }
};
}
}
}

#endif // ENGINE_API_TABLE_PARAMETERS_HPP
