#include "extractor/import_edge.hpp"

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

bool NodeBasedEdge::operator<(const NodeBasedEdge &other) const
{
    if (source == other.source)
    {
        if (target == other.target)
        {
            if (weight == other.weight)
            {
                return forward && backward && ((!other.forward) || (!other.backward));
            }
            return weight < other.weight;
        }
        return target < other.target;
    }
    return source < other.source;
}

NodeBasedEdge::NodeBasedEdge()
    : source(SPECIAL_NODEID), target(SPECIAL_NODEID), name_id(0), weight(0), forward(false),
      backward(false), roundabout(false),
      access_restricted(false), startpoint(true), is_split(false), travel_mode(false)
{
}

NodeBasedEdge::NodeBasedEdge(NodeID source,
                             NodeID target,
                             NodeID name_id,
                             EdgeWeight weight,
                             bool forward,
                             bool backward,
                             bool roundabout,
                             bool access_restricted,
                             bool startpoint,
                             TravelMode travel_mode,
                             bool is_split)
    : source(source), target(target), name_id(name_id), weight(weight), forward(forward),
      backward(backward), roundabout(roundabout),
      access_restricted(access_restricted), startpoint(startpoint), is_split(is_split), travel_mode(travel_mode)
{
}

bool EdgeBasedEdge::operator<(const EdgeBasedEdge &other) const
{
    if (source == other.source)
    {
        if (target == other.target)
        {
            if (weight == other.weight)
            {
                return forward && backward && ((!other.forward) || (!other.backward));
            }
            return weight < other.weight;
        }
        return target < other.target;
    }
    return source < other.source;
}

template <class EdgeT>
EdgeBasedEdge::EdgeBasedEdge(const EdgeT &other)
    : source(other.source), target(other.target), edge_id(other.data.via),
      weight(other.data.distance), forward(other.data.forward), backward(other.data.backward)
{
}

/** Default constructor. target and weight are set to 0.*/
EdgeBasedEdge::EdgeBasedEdge()
    : source(0), target(0), edge_id(0), weight(0), forward(false), backward(false)
{
}

EdgeBasedEdge::EdgeBasedEdge(const NodeID source,
                             const NodeID target,
                             const NodeID edge_id,
                             const EdgeWeight weight,
                             const bool forward,
                             const bool backward)
    : source(source), target(target), edge_id(edge_id), weight(weight), forward(forward),
      backward(backward)
{
}
