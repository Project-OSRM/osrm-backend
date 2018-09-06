#include "engine/routing_algorithms/routing_base.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

bool needsLoopForward(const PhantomNode &source_phantom, const PhantomNode &target_phantom)
{
    return source_phantom.IsValidForwardSource() && target_phantom.IsValidForwardTarget() &&
           source_phantom.forward_segment_id.id == target_phantom.forward_segment_id.id &&
           source_phantom.GetForwardWeightPlusOffset() >
               target_phantom.GetForwardWeightPlusOffset();
}

bool needsLoopBackwards(const PhantomNode &source_phantom, const PhantomNode &target_phantom)
{
    return source_phantom.IsValidReverseSource() && target_phantom.IsValidReverseTarget() &&
           source_phantom.reverse_segment_id.id == target_phantom.reverse_segment_id.id &&
           source_phantom.GetReverseWeightPlusOffset() >
               target_phantom.GetReverseWeightPlusOffset();
}

bool needsLoopForward(const PhantomNodes &phantoms)
{
    return needsLoopForward(phantoms.source_phantom, phantoms.target_phantom);
}

bool needsLoopBackwards(const PhantomNodes &phantoms)
{
    return needsLoopBackwards(phantoms.source_phantom, phantoms.target_phantom);
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
