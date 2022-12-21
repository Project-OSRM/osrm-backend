#ifndef OSRM_EXTRACT_DATASOURCES_HPP
#define OSRM_EXTRACT_DATASOURCES_HPP

#include "util/typedefs.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace osrm::extractor
{

class Datasources
{
    static constexpr const std::uint8_t MAX_NUM_SOURES = 255;
    static constexpr const std::uint8_t MAX_LENGTH_NAME = 255;

  public:
    Datasources()
    {
        std::fill(lengths.begin(), lengths.end(), 0);
        std::fill(sources.begin(), sources.end(), '\0');
    }

    std::string_view GetSourceName(DatasourceID id) const
    {
        auto begin = sources.data() + (MAX_LENGTH_NAME * id);

        return std::string_view{begin, lengths[id]};
    }

    void SetSourceName(DatasourceID id, const std::string &name)
    {
        BOOST_ASSERT(name.size() < MAX_LENGTH_NAME);
        lengths[id] = std::min<std::uint32_t>(name.size(), MAX_LENGTH_NAME);

        auto out = sources.data() + (MAX_LENGTH_NAME * id);
        std::copy(name.begin(), name.begin() + lengths[id], out);
    }

  private:
    std::array<std::uint32_t, MAX_NUM_SOURES> lengths;
    std::array<char, MAX_LENGTH_NAME * MAX_NUM_SOURES> sources;
};
} // namespace osrm::extractor

#endif
