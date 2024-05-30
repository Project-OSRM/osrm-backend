#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <tuple>
#include <utility>

namespace mapbox
{

namespace geometry
{
template <typename T> struct point
{
    using coordinate_type = T;

    constexpr point() : x(), y() {}
    constexpr point(T x_, T y_) : x(x_), y(y_) {}

    T x;
    T y;
};
} // namespace geometry

namespace cheap_ruler
{

using point = geometry::point<double>;

class CheapRuler
{

    // Values that define WGS84 ellipsoid model of the Earth
    static constexpr double RE = 6378.137;            // equatorial radius
    static constexpr double FE = 1.0 / 298.257223563; // flattening

    static constexpr double E2 = FE * (2 - FE);
    static constexpr double RAD = std::numbers::pi / 180.0;

  public:
    explicit CheapRuler(double latitude)
    {
        // Curvature formulas from https://en.wikipedia.org/wiki/Earth_radius#Meridional
        double mul = RAD * RE * 1000;
        double coslat = std::cos(latitude * RAD);
        double w2 = 1 / (1 - E2 * (1 - coslat * coslat));
        double w = std::sqrt(w2);

        // multipliers for converting longitude and latitude degrees into distance
        kx = mul * w * coslat;        // based on normal radius of curvature
        ky = mul * w * w2 * (1 - E2); // based on meridonal radius of curvature
    }

    double squareDistance(point a, point b) const
    {
        auto dx = longDiff(a.x, b.x) * kx;
        auto dy = (a.y - b.y) * ky;
        return dx * dx + dy * dy;
    }

    //
    // Given two points of the form [x = longitude, y = latitude], returns the distance.
    //
    double distance(point a, point b) const { return std::sqrt(squareDistance(a, b)); }

    //
    // Returns the bearing between two points in angles.
    //
    double bearing(point a, point b) const
    {
        auto dx = longDiff(b.x, a.x) * kx;
        auto dy = (b.y - a.y) * ky;

        return std::atan2(dx, dy) / RAD;
    }

  private:
    double ky;
    double kx;
    static double longDiff(double a, double b) { return std::remainder(a - b, 360); }
};

} // namespace cheap_ruler
} // namespace mapbox
