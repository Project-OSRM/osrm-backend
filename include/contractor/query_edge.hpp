#ifndef QUERYEDGE_HPP
#define QUERYEDGE_HPP

#include "util/payload.hpp"
#include "util/typedefs.hpp"

#include <tuple>

namespace osrm
{
namespace contractor
{

struct QueryEdge
{
    NodeID source;
    NodeID target;
    struct EdgeData
    {
        explicit EdgeData()
            : turn_id(0), shortcut(false), weight(0), forward(false), backward(false), payload()
        {
        }

        template <class OtherT> EdgeData(const OtherT &other)
        {
            weight = other.weight;
            shortcut = other.shortcut;
            turn_id = other.id;
            forward = other.forward;
            backward = other.backward;
            payload = other.payload;
        }
        // this ID is either the middle node of the shortcut, or the ID of the edge based node (node
        // based edge) storing the appropriate data. If `shortcut` is set to true, we get the middle
        // node. Otherwise we see the edge based node to access node data.
        NodeID turn_id : 31;
        bool shortcut : 1;
        EdgeWeight weight : 30;
        std::uint32_t forward : 1;
        std::uint32_t backward : 1;
        EdgePayload payload;
    } data;

    QueryEdge() : source(SPECIAL_NODEID), target(SPECIAL_NODEID) {}

    QueryEdge(NodeID source, NodeID target, EdgeData data)
        : source(source), target(target), data(std::move(data))
    {
    }

    bool operator<(const QueryEdge &rhs) const
    {
        return std::tie(source, target) < std::tie(rhs.source, rhs.target);
    }

    bool operator==(const QueryEdge &right) const
    {
        return (source == right.source && target == right.target &&
                data.weight == right.data.weight && data.shortcut == right.data.shortcut &&
                data.forward == right.data.forward && data.backward == right.data.backward &&
                data.payload == right.data.payload && data.turn_id == right.data.turn_id);
    }
};
}
}

#endif // QUERYEDGE_HPP
