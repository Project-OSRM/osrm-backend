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

void insertSourceInHeap(SearchEngineData<ch::Algorithm>::ManyToManyQueryHeap &heap,
                        const PhantomNode &phantom_node)
{
    if (phantom_node.IsValidForwardSource())
    {
        heap.Insert(phantom_node.forward_segment_id.id,
                    -phantom_node.GetForwardWeightPlusOffset(),
                    {phantom_node.forward_segment_id.id, -phantom_node.GetForwardDuration()});
    }
    if (phantom_node.IsValidReverseSource())
    {
        heap.Insert(phantom_node.reverse_segment_id.id,
                    -phantom_node.GetReverseWeightPlusOffset(),
                    {phantom_node.reverse_segment_id.id, -phantom_node.GetReverseDuration()});
    }
}

void insertTargetInHeap(SearchEngineData<ch::Algorithm>::ManyToManyQueryHeap &heap,
                        const PhantomNode &phantom_node)
{
    if (phantom_node.IsValidForwardTarget())
    {
        heap.Insert(phantom_node.forward_segment_id.id,
                    phantom_node.GetForwardWeightPlusOffset(),
                    {phantom_node.forward_segment_id.id, phantom_node.GetForwardDuration()});
    }
    if (phantom_node.IsValidReverseTarget())
    {
        heap.Insert(phantom_node.reverse_segment_id.id,
                    phantom_node.GetReverseWeightPlusOffset(),
                    {phantom_node.reverse_segment_id.id, phantom_node.GetReverseDuration()});
    }
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
