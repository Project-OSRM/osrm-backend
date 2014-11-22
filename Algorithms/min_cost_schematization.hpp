#ifndef MIN_COST_SCHEMATIZATION_H_
#define MIN_COST_SCHEMATIZATION_H_

#include <boost/assert.hpp>
#include <boost/range/irange.hpp>

#include "../DataStructures/SchematizedPlane.h"
#include "../DataStructures/SymbolicCoordinate.h"

/**
 * Schematizes an x-axismontone path with minum schematization cost.
 *
 * This class implements a variant of the SPS methode described by Dellinger et al.
 *
 */
class MinCostSchematization
{
public:

    MinCostSchematization(const SchematizedPlane& plane);

    /*
     * Computes a valid d-regular embedding of the subpath.
     */
    void schematize(SubPath& subpath);

    void computeEdges(std::vector<SymbolicCoordinate>& nodes,
                      std::vector<unsigned>& prefered_directions,
                      std::vector<unsigned>& alternative_directions,
                      std::vector<SubPath::EdgeType>& types) const;

    void setupYOrder(const std::vector<SymbolicCoordinate>& nodes,
                     std::vector<unsigned>& y_order_to_node,
                     std::vector<unsigned>& node_to_y_order) const;

    void partionIntoStripes(const std::vector<unsigned>& y_order_to_node,
                            const std::vector<SymbolicCoordinate>& nodes,
                            std::vector<SubPath::Stripe>& stripes,
                            std::vector<unsigned>& y_order_to_stripe) const;

    void computeMinSchematizationCost(const std::vector<unsigned>& y_order_to_node,
                                      const std::vector<unsigned>& node_to_y_order,
                                      const std::vector<SubPath::Stripe>& stripes,
                                      const std::vector<unsigned>& y_order_to_stripe,
                                      const std::vector<SubPath::EdgeType>& edge_types,
                                      std::vector<bool>& expand_stripe) const;

    void mergeStripes(const std::vector<bool>& expand_stripe,
                      std::vector<unsigned>& y_order_to_stripe,
                      std::vector<SubPath::Stripe>& stripes) const;

    void computeStripeYCoordinates(const std::vector<unsigned>& y_order_to_node,
                                   const std::vector<unsigned>& node_to_y_order,
                                   const std::vector<unsigned>& y_order_to_stripe,
                                   const std::vector<SubPath::Stripe>& stripes,
                                   const std::vector<SymbolicCoordinate>& nodes,
                                   const std::vector<unsigned>& prefered_directions,
                                   std::vector<double>& stripe_y) const;

    void applyStripeHeights(const std::vector<unsigned>& y_order_to_node,
                            const std::vector<unsigned>& y_order_to_stripe,
                            const std::vector<double>& stripe_y,
                            std::vector<unsigned>& prefered_directions,
                            std::vector<SymbolicCoordinate>& nodes) const;

    /**
     * Computes the costs of expanding the current stripe and not expanding all
     * stripe above.
     */
    inline void computeEdgeCosts(unsigned stripe_idx,
                                 const SubPath::Stripe& stripe,
                                 const std::vector<unsigned>& y_order_to_node,
                                 const std::vector<unsigned>& node_to_y_order,
                                 const std::vector<unsigned>& y_order_to_stripe,
                                 const std::vector<SubPath::EdgeType>& edge_types,
                                 std::vector<unsigned>& horizontal_cost,
                                 std::vector<unsigned>& vertical_cost,
                                 std::vector<unsigned>& strictly_vertical_cost) const
    {
        // compute edge costs for all edges that start on lower border of this stripe
        for (const auto& y_idx : stripe)
        {
            unsigned node_id = y_order_to_node[y_idx];

            if (node_id > 0)
            {
                unsigned end_stripe = y_order_to_stripe[node_to_y_order[node_id-1]];

                if (end_stripe <= stripe_idx)
                {
                    addCostForEndpoint(stripe_idx, end_stripe, edge_types[node_id-1],
                                       horizontal_cost,
                                       vertical_cost,
                                       strictly_vertical_cost);
                }
            }
            if (node_id < node_to_y_order.size()-1)
            {
                unsigned end_stripe = y_order_to_stripe[node_to_y_order[node_id+1]];

                if (end_stripe <= stripe_idx)
                {
                    addCostForEndpoint(stripe_idx, end_stripe, edge_types[node_id],
                                       horizontal_cost,
                                       vertical_cost,
                                       strictly_vertical_cost);
                }
            }
        }
    }

    // Important: This function assumes that C[0, stripe_idx-1] .. C[stripe_idx-1, stripe_idx-1]
    // is contained in the costs array!
    // computes the costs C[0, stripe_idx] .. C[stripe_idx, stripe_idx]
    inline void getStripeCosts(const std::vector<unsigned>& horizontal_cost,
                               const std::vector<unsigned>& vertical_cost,
                               const std::vector<unsigned>& strictly_vertical_cost,
                               unsigned stripe_idx,
                               std::vector<unsigned>& costs) const
    {
        for (auto k : boost::irange(0u, stripe_idx))
        {
            // Can't expand this stripe: We would hide a strictly vertical edge
            if (strictly_vertical_cost[k] > 0)
            {
                costs[k] = std::numeric_limits<unsigned>::max();
            }
            else
            {
                // already at max
                if (costs[k] == std::numeric_limits<unsigned>::max())
                {
                    continue;
                }
                // note: costs[k] is C[k, stripe_idx-1]
                costs[k] = costs[k] + horizontal_cost[k] + vertical_cost[k];
            }
        }

        // special case: Expand the current stripe
        if (strictly_vertical_cost[stripe_idx] > 0)
        {
            costs[stripe_idx] = std::numeric_limits<unsigned>::max();
        }
        else
        {
            costs[stripe_idx] = horizontal_cost[stripe_idx];
        }
    }

    /**
     * Adds the costs that would be incured if by the given edge if stripe k was expanded
     *
     * Note that start_stripe <= end_stripe
     */
    inline void addCostForEndpoint(unsigned start_stripe, unsigned end_stripe,
                                   SubPath::EdgeType edgeType,
                                   std::vector<unsigned>& horizontal_cost,
                                   std::vector<unsigned>& vertical_cost,
                                   std::vector<unsigned>& strictly_vertical_cost) const
    {
        switch (edgeType)
        {
            case SubPath::EDGE_HORIZONTAL:
                {
                    // Expanding any stripe between start and end point makes
                    // realizing a horizontal edge impossible
                    for (unsigned k = end_stripe+1; k <= start_stripe; k++)
                    {
                        horizontal_cost[k]++;
                    }
                    break;
                }
            case SubPath::EDGE_VERTICAL:
                {
                    // Expanding any stripe below the endpoint would mean
                    // we don't expand any of the stripes in [k,..,start_stripe]
                    // so the vertical edge could not be realized
                    for (unsigned k = 0; k <= end_stripe; k++)
                    {
                        vertical_cost[k]++;
                    }
                    break;
                }
            case SubPath::EDGE_STRICTLY_VERTICAL:
                {
                    // Same as for vertical edges
                    for (unsigned k = 0; k <= end_stripe; k++)
                    {
                        strictly_vertical_cost[k]++;
                    }
                    break;
                }
            default:
                break;
        }
    }

    /**
     * Classify edge in 4 categories, as schematization cost are the same for each category.
     */
    inline SubPath::EdgeType classifyEdge(const SymbolicCoordinate& a, const SymbolicCoordinate& b, unsigned angle) const
    {
        // horizontal
        if (plane.isHorizontal(angle))
        {
            if (a.y == b.y)
            {
                return SubPath::EDGE_STRICTLY_HORIZONTAL;
            }
            else
            {
                return SubPath::EDGE_HORIZONTAL;
            }

        }
        else
        {
            // we derive from the paper here,
            // as having the same x coordinate practically never happens
            // but this edge would be completly invisiable if the stripe
            // is not expaned
            if (plane.isVertical(angle))
            {
                return SubPath::EDGE_STRICTLY_VERTICAL;
            }
            else
            {
                return SubPath::EDGE_VERTICAL;
            }
        }
    }

private:
    const SchematizedPlane& plane;
};

#endif
