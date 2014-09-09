#ifndef TEST_UTILS_H_
#define TEST_UTILS_H_

#include <osrm/Coordinate.h>

#include "../../DataStructures/SymbolicCoordinate.h"

class TestRaster
{
public:
    TestRaster(double min_lat, double max_lat, double min_lon, double max_lon, unsigned x_res, unsigned y_res)
    : min_lat(min_lat)
    , min_lon(min_lon)
    , scaling(std::max(max_lon - min_lon, static_cast<double>(lat2y(max_lat) - lat2y(min_lat))))
    , x_res(x_res)
    , y_res(y_res)
    {
    }

    SymbolicCoordinate symbolic(unsigned x, unsigned y) const
    {
        BOOST_ASSERT(x <= x_res && y <= y_res);

        return SymbolicCoordinate(x / static_cast<double>(x_res),
                                  y / static_cast<double>(y_res));
    }

    FixedPointCoordinate real(unsigned x, unsigned y) const
    {
        SymbolicCoordinate sym = symbolic(x, y);

        return transformSymbolicToFixed(sym, scaling, FixedPointCoordinate(min_lat, min_lon));
    }

private:
    const double min_lat;
    const double min_lon;
    const double scaling;
    const unsigned x_res;
    const unsigned y_res;
};

#endif
