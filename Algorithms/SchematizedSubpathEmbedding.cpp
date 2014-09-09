#include "SchematizedSubpathEmbedding.h"

#include "../DataStructures/SymbolicCoordinate.h"
#include "../DataStructures/SchematizedPlane.h"
#include "../DataStructures/SubPath.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>

SchematizedSubPathEmbedding::SchematizedSubPathEmbedding(const SchematizedPlane& plane)
: plane(plane)
{
}

void SchematizedSubPathEmbedding::embedd(std::vector<SubPath>& subpaths, std::vector<SymbolicCoordinate>& mergedPath) const
{
    computeDeltas(subpaths);

    mergeSubpaths(subpaths, mergedPath);
}

void SchematizedSubPathEmbedding::computeDeltas(std::vector<SubPath>& subpaths) const
{
    subpaths[0].delta_x = 0;
    subpaths[0].delta_y = 0;
    for (unsigned i = 0; i < subpaths.size() - 1; i++)
    {
        SubPath& current_subpath = subpaths[i];
        SubPath& next_subpath    = subpaths[i+1];

        unsigned real_out_direction = plane.fromXMonotoneDirection(current_subpath.prefered_directions.back(),
                                                                   current_subpath.monoticity);
        unsigned real_in_direction  = plane.fromXMonotoneDirection(next_subpath.prefered_directions.front(),
                                                                   next_subpath.monoticity);

        // connecting the subpaths would cause a folding effect, where
        // two edges are exactly overlayed
        if (plane.isReversed(real_in_direction, real_out_direction))
        {
            // For simplicity we always changed the last edge of the current subpath
            // the alternative direction will preserve monoticity of the subpath
            // but using it will probably destroy the y-order
            unsigned alternative_out_direction = plane.fromXMonotoneDirection(current_subpath.alternative_directions.back(),
                                                                              current_subpath.monoticity);

            BOOST_ASSERT(current_subpath.nodes.size() > 1);
            const SymbolicCoordinate& a = current_subpath.nodes[current_subpath.nodes.size() - 2];
            SymbolicCoordinate& b = current_subpath.nodes.back();

            // preserve the edge length in the hope that we get near the original y-order
            double length = sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y));
            double dx, dy;
            plane.getAngleOffsets(alternative_out_direction, length, dx, dy);

            b.x = a.x + dx;
            b.y = a.y + dy;
        }

        next_subpath.delta_x = current_subpath.delta_x + current_subpath.nodes.back().x - next_subpath.nodes.front().x;
        next_subpath.delta_y = current_subpath.delta_y + current_subpath.nodes.back().y - next_subpath.nodes.front().y;
    }
}

/**
 * This step fixes up the gaps that could be caused by BB placment.
 * instead of the approach in the paper, we don't add edges
 * but instead change egde direction or extent the line length
 */
void SchematizedSubPathEmbedding::mergeSubpaths(const std::vector<SubPath>& subpaths, std::vector<SymbolicCoordinate>& mergedPath) const
{
    // this applies the delta and merges them to one big path
    for (const auto& s : subpaths)
    {
        double delta_x = s.delta_x;
        double delta_y = s.delta_y;
        // copy everything but the last node: It is the same as the first node of the next path.
        std::transform(s.nodes.begin(), s.nodes.end()-1, std::back_inserter(mergedPath),
            [&delta_x, &delta_y](SymbolicCoordinate s)
            {
                s.x += delta_x;
                s.y += delta_y;
                return s;
            });
    }

    // last node was left out
    const SubPath& last_subpath = subpaths.back();
    SymbolicCoordinate last_coord = last_subpath.nodes.back();
    last_coord.x += last_subpath.delta_x;
    last_coord.y += last_subpath.delta_y;
    mergedPath.push_back(last_coord);
}

