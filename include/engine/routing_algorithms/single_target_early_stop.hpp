#ifndef OSRM_ENGINE_ROUTING_ALGORITHMS_SINGLE_TARGET_EARLY_STOP_HPP
#define OSRM_ENGINE_ROUTING_ALGORITHMS_SINGLE_TARGET_EARLY_STOP_HPP

#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace osrm::engine::routing_algorithms::ch
{

class SingleTargetEarlyStop
{
  public:
    void addTargetApproach(const EdgeWeight weight,
                           const EdgeDuration duration,
                           const EdgeDistance distance)
    {
        m_maximum_target_approach =
            std::max(m_maximum_target_approach, from_alias<std::int64_t>(weight));
        m_maximum_target_approach_duration =
            std::max(m_maximum_target_approach_duration, from_alias<std::int64_t>(duration));
        m_maximum_target_approach_distance =
            std::max(m_maximum_target_approach_distance, from_alias<double>(distance));
    }

    void addSourceApproach(const EdgeWeight weight,
                           const EdgeDuration duration,
                           const EdgeDistance distance)
    {
        m_maximum_source_approach =
            std::max(m_maximum_source_approach, from_alias<std::int64_t>(weight));
        m_maximum_source_approach_duration =
            std::max(m_maximum_source_approach_duration, from_alias<std::int64_t>(duration));
        m_maximum_source_approach_distance =
            std::max(m_maximum_source_approach_distance, from_alias<double>(distance));
    }

    void addTarget(const EdgeWeight weight,
                   const EdgeDuration duration,
                   const EdgeDistance distance,
                   const EdgeWeight approach)
    {
        m_minimum_target_weight = std::min(m_minimum_target_weight, weight);
        m_minimum_target_duration = std::min(m_minimum_target_duration, duration);
        m_minimum_target_distance = std::min(m_minimum_target_distance, distance);
        m_maximum_target_approach =
            std::max(m_maximum_target_approach, from_alias<std::int64_t>(approach));
    }

    void
    addSource(const EdgeDuration duration, const EdgeDistance distance, const EdgeWeight approach)
    {
        m_maximum_source_approach =
            std::max(m_maximum_source_approach, from_alias<std::int64_t>(approach));
        insertLabel(duration, distance);
    }

    void insertLabel(const EdgeDuration duration, const EdgeDistance distance)
    { m_pending_negative_metrics += hasNegativeMeeting(duration, distance); }

    void removeLabel(const EdgeDuration duration, const EdgeDistance distance)
    {
        if (hasNegativeMeeting(duration, distance))
        {
            BOOST_ASSERT(m_pending_negative_metrics > 0);
            --m_pending_negative_metrics;
        }
    }

    bool shouldStop(const EdgeWeight next_weight, const EdgeWeight best_weight) const
    {
        if (best_weight == INVALID_EDGE_WEIGHT || m_minimum_target_weight == INVALID_EDGE_WEIGHT ||
            m_pending_negative_metrics != 0)
        {
            return false;
        }
        const auto lower_bound = from_alias<std::int64_t>(next_weight) +
                                 from_alias<std::int64_t>(m_minimum_target_weight);
        return lower_bound > from_alias<std::int64_t>(best_weight) &&
               lower_bound >= m_maximum_source_approach + m_maximum_target_approach;
    }

  private:
    bool hasNegativeMeeting(const EdgeDuration duration, const EdgeDistance distance) const
    {
        // Approach maxima are fixed before inserting labels. Nonnegative CH edge
        // metrics cannot create a new negative graph metric after these labels settle.
        return from_alias<std::int64_t>(duration) +
                       from_alias<std::int64_t>(m_minimum_target_duration) <
                   m_maximum_source_approach_duration + m_maximum_target_approach_duration ||
               from_alias<double>(distance) + from_alias<double>(m_minimum_target_distance) <
                   m_maximum_source_approach_distance + m_maximum_target_approach_distance;
    }

    EdgeWeight m_minimum_target_weight = INVALID_EDGE_WEIGHT;
    EdgeDuration m_minimum_target_duration = MAXIMAL_EDGE_DURATION;
    EdgeDistance m_minimum_target_distance = MAXIMAL_EDGE_DISTANCE;
    std::int64_t m_maximum_target_approach = 0;
    std::int64_t m_maximum_source_approach = 0;
    std::int64_t m_maximum_target_approach_duration = 0;
    std::int64_t m_maximum_source_approach_duration = 0;
    double m_maximum_target_approach_distance = 0;
    double m_maximum_source_approach_distance = 0;
    std::size_t m_pending_negative_metrics = 0;
};

} // namespace osrm::engine::routing_algorithms::ch

#endif
