#ifndef QUERYEDGE_HPP
#define QUERYEDGE_HPP

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
        EdgeData() : id(0), shortcut(false), weight(0), forward(false), backward(false) {}

        template <class OtherT> EdgeData(const OtherT &other)
        {
            weight = other.weight;
            shortcut = other.shortcut;
            id = other.id;
            forward = other.forward;
            backward = other.backward;
        }
        NodeID id : 31;
        bool shortcut : 1;
        int weight : 30;
        bool forward : 1;
        bool backward : 1;
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
                data.id == right.data.id);
    }
};
}
}

#endif // QUERYEDGE_HPP
