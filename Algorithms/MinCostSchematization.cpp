#include "MinCostSchematization.h"

#include "../Util/MercatorUtil.h"
#include "../Util/TimingUtil.h"

#include <boost/assert.hpp>
#include <boost/range/irange.hpp>

MinCostSchematization::MinCostSchematization(const SchematizedPlane& plane)
: plane(plane)
{
}

/*
 * Computes a valid d-regular embedding of the subpath.
 */
void MinCostSchematization::schematize(SubPath& subpath)
{
    computeEdges(subpath.nodes, subpath.prefered_directions, subpath.alternative_directions, subpath.edge_types);

    setupYOrder(subpath.nodes, subpath.y_order_to_node, subpath.node_to_y_order);

    std::vector<unsigned> y_order_to_stripe;
    partionIntoStripes(subpath.y_order_to_node, subpath.nodes, subpath.stripes, y_order_to_stripe);

    std::vector<bool> expand_stripe;
    computeMinSchematizationCost(subpath.y_order_to_node,
                                 subpath.node_to_y_order,
                                 subpath.stripes,
                                 y_order_to_stripe,
                                 subpath.edge_types,
                                 expand_stripe);

    mergeStripes(expand_stripe,
                 y_order_to_stripe,
                 subpath.stripes);

    std::vector<double> stripe_y;
    computeStripeYCoordinates(subpath.y_order_to_node,
                              subpath.node_to_y_order,
                              y_order_to_stripe,
                              subpath.stripes,
                              subpath.nodes,
                              subpath.prefered_directions,
                              stripe_y);

    applyStripeHeights(subpath.y_order_to_node, y_order_to_stripe, stripe_y, subpath.prefered_directions, subpath.nodes);
}

void MinCostSchematization::computeEdges(std::vector<SymbolicCoordinate>& nodes,
                                         std::vector<unsigned>& prefered_directions,
                                         std::vector<unsigned>& alternative_directions,
                                         std::vector<SubPath::EdgeType>& types) const
{
    // Empirically the number of edges will be reduced wastly by edge simplification
    prefered_directions.reserve(nodes.size() / 2);
    alternative_directions.reserve(nodes.size() / 2);
    types.reserve(nodes.size() / 2);

    std::vector<SymbolicCoordinate> reduced_nodes;
    reduced_nodes.reserve(nodes.size() / 2);

    unsigned last_direction = std::numeric_limits<unsigned>::max();
    unsigned last_interval  = std::numeric_limits<unsigned>::max();
    for (unsigned i = 0; i < nodes.size() - 1; i++)
    {
        double angle = plane.getAngle(nodes[i], nodes[i+1]);
        unsigned prefered_direction;
        unsigned alternative_direction;
        plane.getPreferedDirections(angle, prefered_direction, alternative_direction);

        if (prefered_direction != last_direction || nodes[i].interval_id != last_interval)
        {
            prefered_directions.push_back(prefered_direction);
            alternative_directions.push_back(alternative_direction);
            types.push_back(classifyEdge(nodes[i], nodes[i+1], prefered_direction));
            reduced_nodes.push_back(nodes[i]);
            last_direction = prefered_direction;
            last_interval = nodes[i].interval_id;
        }
    }
    reduced_nodes.push_back(nodes.back());

    nodes.swap(reduced_nodes);

    // note: nodes changed its size!
    BOOST_ASSERT(nodes.size()-1 == prefered_directions.size());
    BOOST_ASSERT(nodes.size()-1 == alternative_directions.size());
    BOOST_ASSERT(nodes.size()-1 == types.size());
}

void MinCostSchematization::setupYOrder(const std::vector<SymbolicCoordinate>& nodes,
                                        std::vector<unsigned>& y_order_to_node,
                                        std::vector<unsigned>& node_to_y_order) const
{
    y_order_to_node.resize(nodes.size());
    node_to_y_order.resize(nodes.size());
    std::iota(y_order_to_node.begin(), y_order_to_node.end(), 0);
    // sort by y coordinate.
    // Note: Since we do not use the image coordinate system (y positive downwards)
    // but the mathimatical coordinate system (y positive upwards)
    // we will use the term *upper bound* to refer to lower y-coordinates and *lower bound*
    // respectively.
    std::sort(y_order_to_node.begin(), y_order_to_node.end(),
        [&nodes](unsigned a, unsigned b)
        {
            return nodes[a].y < nodes[b].y;
        });
    for (unsigned i = 0; i < nodes.size(); i++)
    {
        node_to_y_order[y_order_to_node[i]] = i;
    }
}

void MinCostSchematization::partionIntoStripes(const std::vector<unsigned>& y_order_to_node,
                                               const std::vector<SymbolicCoordinate>& nodes,
                                               std::vector<SubPath::Stripe>& stripes,
                                               std::vector<unsigned>& y_order_to_stripe) const
{
    y_order_to_stripe.resize(nodes.size());

    unsigned stripe_start = 0;
    unsigned stripe_idx = 0;
    // pleasee note start stripe 0 only has a lower bound
    for (unsigned i = 0; i < nodes.size() - 1; i++)
    {
        y_order_to_stripe[i] = stripe_idx;

        if (std::abs(nodes[y_order_to_node[i]].y - nodes[y_order_to_node[i+1]].y)
            > std::numeric_limits<double>::epsilon())
        {
            stripes.push_back(boost::irange(stripe_start, i+1));
            stripe_start = i+1;
            stripe_idx++;
        }
    }
    stripes.push_back(boost::irange(stripe_start, static_cast<unsigned>(nodes.size())));
    y_order_to_stripe[nodes.size()-1] = stripes.size() - 1;
}

void MinCostSchematization::computeMinSchematizationCost(const std::vector<unsigned>& y_order_to_node,
                                                         const std::vector<unsigned>& node_to_y_order,
                                                         const std::vector<SubPath::Stripe>& stripes,
                                                         const std::vector<unsigned>& y_order_to_stripe,
                                                         const std::vector<SubPath::EdgeType>& edge_types,
                                                         std::vector<bool>& expand_stripe) const
{
    unsigned num_stripes = stripes.size();

    std::vector<unsigned> min_index(num_stripes, 0);
    // at each iteration i these are d_q(k,i) for q \in {H, V, SV}
    // we only use the first i elements in each iteration.
    std::vector<unsigned> horizontal_cost(num_stripes, 0);
    std::vector<unsigned> vertical_cost(num_stripes, 0);
    std::vector<unsigned> strictly_vertical_cost(num_stripes, 0);

    std::vector<unsigned> stripe_cost(num_stripes, 0);
    std::vector<unsigned> min_schematization_cost(num_stripes, 0);

    // compute min_schematization_cost[i] for stripe i
    for (auto i : boost::irange(0u, num_stripes))
    {
        computeEdgeCosts(i,
                         stripes[i],
                         y_order_to_node,
                         node_to_y_order,
                         y_order_to_stripe,
                         edge_types,
                         horizontal_cost,
                         vertical_cost,
                         strictly_vertical_cost);

        // compute C[0, i], ..., C[i, i]
        getStripeCosts(horizontal_cost, vertical_cost, strictly_vertical_cost, i, stripe_cost);

        unsigned min_cost = std::numeric_limits<unsigned>::max();
        unsigned min_idx = i;
        for (unsigned k = 0; k <= i; k++)
        {
            unsigned T = k > 0 ? min_schematization_cost[k-1] : 0;
            unsigned C = i > 0 ? stripe_cost[k] : 0;
            unsigned sum;
            if (T != std::numeric_limits<unsigned>::max()
             && C != std::numeric_limits<unsigned>::max())
            {
                sum = T + C;
            }
            else
            {
                sum = std::numeric_limits<unsigned>::max();
            }

            if (sum < min_cost)
            {
                min_cost = sum;
                min_idx = k;
            }
        }
        if (min_cost )
        min_schematization_cost[i] = min_cost;
        min_index[i] = min_idx;

        std::fill(horizontal_cost.begin(),        horizontal_cost.begin()        + i+1, 0);
        std::fill(vertical_cost.begin(),          vertical_cost.begin()          + i+1, 0);
        std::fill(strictly_vertical_cost.begin(), strictly_vertical_cost.begin() + i+1, 0);
    }

    // new backtrack an construct
    expand_stripe.resize(stripes.size());
    std::fill(expand_stripe.begin(), expand_stripe.end(), false);
    unsigned current_min_idx = min_index.back();
    while (current_min_idx > 0)
    {
        expand_stripe[current_min_idx] = true;
        current_min_idx = min_index[current_min_idx-1];
    }
    expand_stripe[0] = true;
}

void MinCostSchematization::mergeStripes(const std::vector<bool>& expand_stripe,
                                         std::vector<unsigned>& y_order_to_stripe,
                                         std::vector<SubPath::Stripe>& stripes) const
{
    std::vector<SubPath::Stripe> merged_stripes;
    unsigned last_start = 0;
    for (unsigned stripe_idx = 1; stripe_idx < stripes.size(); stripe_idx++)
    {
        if (expand_stripe[stripe_idx])
        {
            const SubPath::Stripe& s = stripes[stripe_idx];
            const SubPath::Stripe new_stripe = boost::irange(last_start, static_cast<unsigned>(s.front()));
            merged_stripes.push_back(new_stripe);
            last_start = s.front();

            // fixup lookup table
            for (const auto idx : new_stripe)
            {
                y_order_to_stripe[idx] = merged_stripes.size() - 1;
            }
        }
    }
    const SubPath::Stripe last_stripe = boost::irange(last_start, static_cast<unsigned>(y_order_to_stripe.size()));
    merged_stripes.push_back(last_stripe);

    for (const auto idx : last_stripe)
    {
        y_order_to_stripe[idx] = merged_stripes.size() - 1;
    }

    stripes.swap(merged_stripes);
}

void MinCostSchematization::computeStripeYCoordinates(const std::vector<unsigned>& y_order_to_node,
                                                      const std::vector<unsigned>& node_to_y_order,
                                                      const std::vector<unsigned>& y_order_to_stripe,
                                                      const std::vector<SubPath::Stripe>& stripes,
                                                      const std::vector<SymbolicCoordinate>& nodes,
                                                      const std::vector<unsigned>& prefered_directions,
                                                      std::vector<double>& stripe_y) const
{


    // compute real y-coordinates of the lower bounds of each stripe
    stripe_y.resize(stripes.size());
    stripe_y[0] = 0.0;
    for (unsigned stripe_idx = 1; stripe_idx < stripes.size(); stripe_idx++)
    {
        stripe_y[stripe_idx] = stripe_y[stripe_idx - 1];
        double max_y = 0;
        for (auto idx : stripes[stripe_idx])
        {
            unsigned node_on_stripe = y_order_to_node[idx];
            // compute minimum height for incoming edge
            if (node_on_stripe > 0)
            {
                unsigned start_node = node_on_stripe - 1;
                unsigned start_stripe = y_order_to_stripe[node_to_y_order[start_node]];
                // in case of stripe_idx == start_stripe, we have horizontal egde,
                // which we can ignore
                if (start_stripe < stripe_idx)
                {
                    double dy = stripe_y[stripe_idx] - stripe_y[start_stripe];
                    double new_dy = plane.extendToMinLenEdge(prefered_directions[start_node], dy);
                    double new_y = stripe_y[start_stripe] + std::abs(new_dy);
                    if (new_y > max_y)
                    {
                        max_y = new_y;
                    }
                }
            }

            // compute minimum height for outgoing edge
            if (node_on_stripe < y_order_to_node.size() - 1)
            {
                unsigned end_node = node_on_stripe + 1;
                unsigned end_stripe = y_order_to_stripe[node_to_y_order[end_node]];
                // in case of stripe_idx == end_stripe, we have horizontal egde,
                // which we can ignore
                if (end_stripe < stripe_idx)
                {
                    double dy = stripe_y[stripe_idx] - stripe_y[end_stripe];
                    double new_dy = plane.extendToMinLenEdge(prefered_directions[node_on_stripe], dy);
                    double new_y = stripe_y[end_stripe] + std::abs(new_dy);
                    if (new_y > max_y)
                    {
                        max_y = new_y;
                    }
                }
            }
        }

        stripe_y[stripe_idx] = std::max(stripe_y[stripe_idx], max_y);
    }
}

void MinCostSchematization::applyStripeHeights(const std::vector<unsigned>& y_order_to_node,
                                               const std::vector<unsigned>& y_order_to_stripe,
                                               const std::vector<double>& stripe_y,
                                               std::vector<unsigned>& prefered_directions,
                                               std::vector<SymbolicCoordinate>& nodes) const
{
    // apply y-coordinates
    for (unsigned y_idx = 0; y_idx < nodes.size(); y_idx++)
    {
        nodes[y_order_to_node[y_idx]].y = stripe_y[y_order_to_stripe[y_idx]];
    }

    // finially transform to d-schematization
    double x_offset = nodes[0].x;
    for (unsigned i = 0; i < nodes.size() - 1; i++)
    {
        double d_y = nodes[i+1].y - nodes[i].y;

        unsigned& direction = prefered_directions[i];

        // check for impossible directions (e.g. horizontal edge width d_y > 0)
        if (plane.isHorizontal(direction)
         && std::abs(d_y) > std::numeric_limits<double>::epsilon())
        {
            // this path is x-monotone increasing: other horizontal direction would be invalid!
            BOOST_ASSERT(direction == 0);
            if (d_y > 0)
            {
                direction = plane.nextAngleAntiClockwise(direction);
            }
            else
            {
                direction = plane.nextAngleClockwise(direction);
            }
        }
        // this edge will not have any height: correct direction to horizontal edge
        if (!plane.isHorizontal(direction)
         && std::abs(d_y) < std::numeric_limits<double>::epsilon())
        {
            direction = 0;
        }

        x_offset += plane.getAngleXOffset(direction, d_y);
        nodes[i+1].x = x_offset;
    }
}

