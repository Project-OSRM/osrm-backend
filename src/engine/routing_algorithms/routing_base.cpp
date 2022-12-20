#include "engine/routing_algorithms/routing_base.hpp"

namespace osrm::engine::routing_algorithms
{

bool requiresForwardLoop(const PhantomNode &source, const PhantomNode &target)
{
    return source.IsValidForwardSource() && target.IsValidForwardTarget() &&
           source.forward_segment_id.id == target.forward_segment_id.id &&
           source.GetForwardWeightPlusOffset() > target.GetForwardWeightPlusOffset();
}

bool requiresBackwardLoop(const PhantomNode &source, const PhantomNode &target)
{
    return source.IsValidReverseSource() && target.IsValidReverseTarget() &&
           source.reverse_segment_id.id == target.reverse_segment_id.id &&
           source.GetReverseWeightPlusOffset() > target.GetReverseWeightPlusOffset();
}

std::vector<NodeID> getForwardLoopNodes(const PhantomEndpointCandidates &endpoint_candidates)
{
    std::vector<NodeID> res;
    for (const auto &source_phantom : endpoint_candidates.source_phantoms)
    {
        auto requires_loop =
            std::any_of(endpoint_candidates.target_phantoms.begin(),
                        endpoint_candidates.target_phantoms.end(),
                        [&](const auto &target_phantom) {
                            return requiresForwardLoop(source_phantom, target_phantom);
                        });
        if (requires_loop)
        {
            res.push_back(source_phantom.forward_segment_id.id);
        }
    }
    return res;
}

std::vector<NodeID> getForwardLoopNodes(const PhantomCandidatesToTarget &endpoint_candidates)
{
    std::vector<NodeID> res;
    for (const auto &source_phantom : endpoint_candidates.source_phantoms)
    {
        if (requiresForwardLoop(source_phantom, endpoint_candidates.target_phantom))
        {
            res.push_back(source_phantom.forward_segment_id.id);
        }
    }
    return res;
}

std::vector<NodeID> getBackwardLoopNodes(const PhantomEndpointCandidates &endpoint_candidates)
{
    std::vector<NodeID> res;
    for (const auto &source_phantom : endpoint_candidates.source_phantoms)
    {
        auto requires_loop =
            std::any_of(endpoint_candidates.target_phantoms.begin(),
                        endpoint_candidates.target_phantoms.end(),
                        [&](const auto &target_phantom) {
                            return requiresBackwardLoop(source_phantom, target_phantom);
                        });
        if (requires_loop)
        {
            res.push_back(source_phantom.reverse_segment_id.id);
        }
    }
    return res;
}

std::vector<NodeID> getBackwardLoopNodes(const PhantomCandidatesToTarget &endpoint_candidates)
{
    std::vector<NodeID> res;
    for (const auto &source_phantom : endpoint_candidates.source_phantoms)
    {
        if (requiresBackwardLoop(source_phantom, endpoint_candidates.target_phantom))
        {
            res.push_back(source_phantom.reverse_segment_id.id);
        }
    }
    return res;
}

PhantomEndpoints endpointsFromCandidates(const PhantomEndpointCandidates &candidates,
                                         const std::vector<NodeID> &path)
{
    auto source_it = std::find_if(candidates.source_phantoms.begin(),
                                  candidates.source_phantoms.end(),
                                  [&path](const auto &source_phantom) {
                                      return path.front() == source_phantom.forward_segment_id.id ||
                                             path.front() == source_phantom.reverse_segment_id.id;
                                  });
    BOOST_ASSERT(source_it != candidates.source_phantoms.end());

    auto target_it = std::find_if(candidates.target_phantoms.begin(),
                                  candidates.target_phantoms.end(),
                                  [&path](const auto &target_phantom) {
                                      return path.back() == target_phantom.forward_segment_id.id ||
                                             path.back() == target_phantom.reverse_segment_id.id;
                                  });
    BOOST_ASSERT(target_it != candidates.target_phantoms.end());

    return PhantomEndpoints{*source_it, *target_it};
}

} // namespace osrm::engine::routing_algorithms
