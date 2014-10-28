#include "../Util/MercatorUtil.h"
#include "../DataStructures/SymbolicCoordinate.h"
#include "../DataStructures/SubPath.h"

#include "MonotoneDecomposition.h"

#include <boost/assert.hpp>
#include <boost/range/irange.hpp>

namespace MonotoneDecomposition
{

std::vector<SymbolicCoordinate> getSubPath(const std::vector<SymbolicCoordinate>& path, unsigned first, unsigned last)
{
    return std::vector<SymbolicCoordinate>(path.begin() + first, path.begin() + last);
}

/**
 *  Decomposes a simple (!) path into axis monotone sub-paths.
 *  A subpath is copied from the path to a subpath acording to its monoticity.
 */
void axisMonotoneDecomposition(const std::vector<SymbolicCoordinate>& path, std::vector<SubPath>& subpaths)
{
    BOOST_ASSERT(path.size() >= 2);

    unsigned last_pos = 0;
    PathMonotiticy mono = MONOTONE_CONSTANT;

    for (unsigned i = 0; i < path.size()-1; i++)
    {
        // monoticity of current edge
        PathMonotiticy current_mono = MONOTONE_CONSTANT;
        // not increasing
        if (path[i].y > path[i+1].y)
        {
            current_mono = static_cast<PathMonotiticy>(current_mono & ~MONOTONE_INCREASING_Y);
        }
        if (path[i].x > path[i+1].x)
        {
            current_mono = static_cast<PathMonotiticy>(current_mono & ~MONOTONE_INCREASING_X);
        }
        // not decreasing
        if (path[i].y < path[i+1].y)
        {
            current_mono = static_cast<PathMonotiticy>(current_mono & ~MONOTONE_DECREASING_Y);
        }
        if (path[i].x < path[i+1].x)
        {
            current_mono = static_cast<PathMonotiticy>(current_mono & ~MONOTONE_DECREASING_X);
        }

        // monoticity of subpath last_pos .. i+1
        PathMonotiticy next_mono = static_cast<PathMonotiticy>(mono & current_mono);

        if (next_mono == MONOTONE_INVALID)
        {
            subpaths.emplace_back(getSubPath(path, last_pos, i+1), mono, last_pos, i);
            last_pos = i;
            mono = current_mono;
        }
        else
        {
            mono = next_mono;
        }
    }

    // finish last subpath
    subpaths.emplace_back(getSubPath(path, last_pos, path.size()), mono, last_pos, static_cast<unsigned>(path.size()-1));
}

/**
 * This function transform a path of abitary monotitity to a x-monotone increasing
 * path by rotating/reversing it.
 */
void transformToXMonotoneIncreasing(SubPath& subpath)
{
    BOOST_ASSERT(subpath.monoticity != MONOTONE_INVALID);

    if (subpath.monoticity & MONOTONE_INCREASING_X)
    {
        return;
    }

    // make x-monotone: swap x and y (mirror on (0,0)->(1,1))
    if (subpath.monoticity & MONOTONE_INCREASING_Y || subpath.monoticity & MONOTONE_DECREASING_Y)
    {
        for (unsigned i = 0; i < subpath.nodes.size(); i++)
        {
            std::swap(subpath.nodes[i].y, subpath.nodes[i].x);
        }
    }

    // we are now x-monotone increasing
    if (subpath.monoticity & MONOTONE_INCREASING_Y)
    {
        return;
    }

    // at this point we are always x-monotone descreasing: Mirror on y-Axis
    std::transform(subpath.nodes.begin(), subpath.nodes.end(), subpath.nodes.begin(),
                   [&](SymbolicCoordinate sym)
                   {
                        sym.x *= -1;
                        return sym;
                   });
}

void transformFromXMonotoneIncreasing(SubPath& subpath)
{
    BOOST_ASSERT(subpath.monoticity != MONOTONE_INVALID);

    if (subpath.monoticity & MONOTONE_INCREASING_X)
    {
        return;
    }

    // make y-monotone again
    if (subpath.monoticity & MONOTONE_INCREASING_Y)
    {
        for (unsigned i = 0; i < subpath.nodes.size(); i++)
        {
            std::swap(subpath.nodes[i].y, subpath.nodes[i].x);
        }
        return;
    }

    // reverse mirroring on y-Axis
    std::transform(subpath.nodes.begin(), subpath.nodes.end(), subpath.nodes.begin(),
                   [&](SymbolicCoordinate sym)
                   {
                        sym.x *= -1;
                        return sym;
                   });

    if (subpath.monoticity & MONOTONE_DECREASING_Y)
    {
        for (unsigned i = 0; i < subpath.nodes.size(); i++)
        {
            std::swap(subpath.nodes[i].y, subpath.nodes[i].x);
        }
    }
}

}

