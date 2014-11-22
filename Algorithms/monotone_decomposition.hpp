#ifndef MONOTONE_DECOMPOSITION_H_
#define MONOTONE_DECOMPOSITION_H_

#include "../Util/MercatorUtil.h"

class SymbolicCoordinate;
class SubPath;

#include <boost/assert.hpp>
#include <boost/range/irange.hpp>

/**
 * Functions to decompose simple paths into monotone sub-paths,
 * transform them to be x-monotone increasing and reverse all that.
 *
 * This is a major building block for the SPS path schematization,
 * as it requires x-monotone paths as input.
 *
 */
namespace MonotoneDecomposition
{
    std::vector<SymbolicCoordinate> getSubPath(const std::vector<SymbolicCoordinate>& path, unsigned first, unsigned last);

    void axisMonotoneDecomposition(const std::vector<SymbolicCoordinate>& path, std::vector<SubPath>& subpaths);

    void transformToXMonotoneIncreasing(SubPath& subpath);

    void transformFromXMonotoneIncreasing(SubPath& subpath);
}

#endif
