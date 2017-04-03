#include "engine/routing_algorithms/routing_base.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

bool needsLoopForward(const PhantomNode &source_phantom, const PhantomNode &target_phantom)
{
    return source_phantom.forward_segment_id.enabled && target_phantom.forward_segment_id.enabled &&
           source_phantom.forward_segment_id.id == target_phantom.forward_segment_id.id &&
           source_phantom.GetForwardWeightPlusOffset() >
               target_phantom.GetForwardWeightPlusOffset();
}

bool needsLoopBackwards(const PhantomNode &source_phantom, const PhantomNode &target_phantom)
{
    return source_phantom.reverse_segment_id.enabled && target_phantom.reverse_segment_id.enabled &&
           source_phantom.reverse_segment_id.id == target_phantom.reverse_segment_id.id &&
           source_phantom.GetReverseWeightPlusOffset() >
               target_phantom.GetReverseWeightPlusOffset();
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
