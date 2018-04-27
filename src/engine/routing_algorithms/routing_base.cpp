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

void adjustPathDistanceToPhantomNodes(const std::vector<NodeID> &path,
                                      const PhantomNode &source_phantom,
                                      const PhantomNode &target_phantom,
                                      EdgeDistance &distance)
{
    if (!path.empty())
    {

        // check the direction of travel to figure out how to calculate the offset to/from
        // the source/target
        if (source_phantom.forward_segment_id.id == path.front())
        {
            //       ............       <-- calculateEGBAnnotation returns distance from 0 to 3
            //       -->s               <-- subtract offset to start at source
            //          .........       <-- want this distance as result
            // entry 0---1---2---3---   <-- 3 is exit node
            distance -= source_phantom.GetForwardDistance();
        }
        else if (source_phantom.reverse_segment_id.id == path.front())
        {
            //       ............    <-- calculateEGBAnnotation returns distance from 0 to 3
            //          s<-------    <-- subtract offset to start at source
            //       ...             <-- want this distance
            // entry 0---1---2---3   <-- 3 is exit node
            distance -= source_phantom.GetReverseDistance();
        }
        if (target_phantom.forward_segment_id.id == path.back())
        {
            //       ............       <-- calculateEGBAnnotation returns distance from 0 to 3
            //                   ++>t   <-- add offset to get to target
            //       ................   <-- want this distance as result
            // entry 0---1---2---3---   <-- 3 is exit node
            distance += target_phantom.GetForwardDistance();
        }
        else if (target_phantom.reverse_segment_id.id == path.back())
        {
            //       ............       <-- calculateEGBAnnotation returns distance from 0 to 3
            //                   <++t   <-- add offset to get from target
            //       ................   <-- want this distance as result
            // entry 0---1---2---3---   <-- 3 is exit node
            distance += target_phantom.GetReverseDistance();
        }
    }
    else
    {
        // there is no shortcut to unpack. source and target are on the same EBG Node.
        // if the offset of the target is greater than the offset of the source, subtract it
        if (target_phantom.GetForwardDistance() > source_phantom.GetForwardDistance())
        {
            //       --------->t        <-- offsets
            //       ->s                <-- subtract source offset from target offset
            //         .........        <-- want this distance as result
            // entry 0---1---2---3---   <-- 3 is exit node
            distance = target_phantom.GetForwardDistance() - source_phantom.GetForwardDistance();
        }
        else
        {
            //               s<---      <-- offsets
            //         t<---------      <-- subtract source offset from target offset
            //         ......           <-- want this distance as result
            // entry 0---1---2---3---   <-- 3 is exit node
            distance = target_phantom.GetReverseDistance() - source_phantom.GetReverseDistance();
        }
    }
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
