#ifndef EDGE_BASED_EDGE_HPP
#define EDGE_BASED_EDGE_HPP

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"
#include <tuple>

namespace osrm
{
namespace extractor
{

struct EdgeBasedEdge
{
  public:
    EdgeBasedEdge();

    template <class EdgeT> explicit EdgeBasedEdge(const EdgeT &other);

    EdgeBasedEdge(const NodeID source,
                  const NodeID target,
                  const NodeID edge_id,
                  const EdgeWeight weight,
                  const bool forward,
                  const bool backward);

    bool operator<(const EdgeBasedEdge &other) const;

    NodeID source;
    NodeID target;
    NodeID edge_id;
    EdgeWeight weight : 30;
    std::uint32_t forward : 1;
    std::uint32_t backward : 1;
};
static_assert(sizeof(extractor::EdgeBasedEdge) == 16,
              "Size of extractor::EdgeBasedEdge type is "
              "bigger than expected. This will influence "
              "memory consumption.");

// Impl.

inline EdgeBasedEdge::EdgeBasedEdge()
    : source(0), target(0), edge_id(0), weight(0), forward(false), backward(false)
{
}

template <class EdgeT>
inline EdgeBasedEdge::EdgeBasedEdge(const EdgeT &other)
    : source(other.source), target(other.target), edge_id(other.data.via),
      weight(other.data.distance), forward(other.data.forward), backward(other.data.backward)
{
}

inline EdgeBasedEdge::EdgeBasedEdge(const NodeID source,
                                    const NodeID target,
                                    const NodeID edge_id,
                                    const EdgeWeight weight,
                                    const bool forward,
                                    const bool backward)
    : source(source), target(target), edge_id(edge_id), weight(weight), forward(forward),
      backward(backward)
{
}

inline bool EdgeBasedEdge::operator<(const EdgeBasedEdge &other) const
{
    const auto unidirectional = (!forward || !backward);
    const auto other_is_unidirectional = (!other.forward || !other.backward);
    // if all items are the same, we want to keep bidirectional edges. due to the `<` operator,
    // preferring 0 (false) over 1 (true), we need to compare the inverse of `bidirectional`
    return std::tie(source, target, weight, unidirectional) <
           std::tie(other.source, other.target, other.weight, other_is_unidirectional);
}
} // ns extractor
} // ns osrm

#endif /* EDGE_BASED_EDGE_HPP */
