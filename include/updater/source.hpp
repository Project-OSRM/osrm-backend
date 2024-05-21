#ifndef OSRM_UPDATER_SOURCE_HPP
#define OSRM_UPDATER_SOURCE_HPP

#include "util/typedefs.hpp"

#include <optional>
#include <vector>

namespace osrm::updater
{

template <typename Key, typename Value> struct LookupTable
{
    std::optional<Value> operator()(const Key &key) const
    {
        using Result = std::optional<Value>;
        const auto it =
            std::lower_bound(lookup.begin(),
                             lookup.end(),
                             key,
                             [](const auto &lhs, const auto &rhs) { return rhs < lhs.first; });
        return it != std::end(lookup) && !(it->first < key) ? Result(it->second) : Result();
    }

    std::vector<std::pair<Key, Value>> lookup;
};

struct Segment final
{
    std::uint64_t from, to;
    Segment() : from(0), to(0) {}
    Segment(const std::uint64_t from, const std::uint64_t to) : from(from), to(to) {}
    Segment(const OSMNodeID from, const OSMNodeID to)
        : from(static_cast<std::uint64_t>(from)), to(static_cast<std::uint64_t>(to))
    {
    }

    bool operator<(const Segment &rhs) const
    {
        return std::tie(from, to) < std::tie(rhs.from, rhs.to);
    }
    bool operator==(const Segment &rhs) const
    {
        return std::tie(from, to) == std::tie(rhs.from, rhs.to);
    }
};

struct SpeedSource final
{
    SpeedSource() : speed(0.), rate() {}
    double speed;
    std::optional<double> rate;
    std::uint8_t source;
};

struct Turn final
{
    std::uint64_t from, via, to;
    Turn() : from(0), via(0), to(0) {}
    Turn(const std::uint64_t from, const std::uint64_t via, const std::uint64_t to)
        : from(from), via(via), to(to)
    {
    }
    Turn(const OSMNodeID &from_id, const OSMNodeID &via_id, const OSMNodeID &to_id)
        : from(static_cast<std::uint64_t>(from_id)), via(static_cast<std::uint64_t>(via_id)),
          to(static_cast<std::uint64_t>(to_id))
    {
    }
    bool operator<(const Turn &rhs) const
    {
        return std::tie(from, via, to) < std::tie(rhs.from, rhs.via, rhs.to);
    }
    bool operator==(const Turn &rhs) const
    {
        return std::tie(from, via, to) == std::tie(rhs.from, rhs.via, rhs.to);
    }
};

struct PenaltySource final
{
    PenaltySource() : duration(0.), weight(std::numeric_limits<double>::quiet_NaN()) {}
    double duration;
    double weight;
    std::uint8_t source;
};

using SegmentLookupTable = LookupTable<Segment, SpeedSource>;
using TurnLookupTable = LookupTable<Turn, PenaltySource>;
} // namespace osrm::updater

#endif
