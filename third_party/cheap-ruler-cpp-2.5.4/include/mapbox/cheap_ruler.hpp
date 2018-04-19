#pragma once

#include <mapbox/geometry.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <tuple>
#include <utility>

namespace mapbox {
namespace cheap_ruler {

using box               = geometry::box<double>;
using line_string       = geometry::line_string<double>;
using linear_ring       = geometry::linear_ring<double>;
using multi_line_string = geometry::multi_line_string<double>;
using point             = geometry::point<double>;
using polygon           = geometry::polygon<double>;

class CheapRuler {
public:
    enum Unit {
        Kilometers,
        Miles,
        NauticalMiles,
        Meters,
        Metres = Meters,
        Yards,
        Feet,
        Inches
    };

    //
    // A collection of very fast approximations to common geodesic measurements. Useful
    // for performance-sensitive code that measures things on a city scale. Point coordinates
    // are in the [x = longitude, y = latitude] form.
    //
    explicit CheapRuler(double latitude, Unit unit = Kilometers) {
        double m = 0.;

        switch (unit) {
        case Kilometers:
            m = 1.;
            break;
        case Miles:
            m = 1000. / 1609.344;
            break;
        case NauticalMiles:
            m = 1000. / 1852.;
            break;
        case Meters:
            m = 1000.;
            break;
        case Yards:
            m = 1000. / 0.9144;
            break;
        case Feet:
            m = 1000. / 0.3048;
            break;
        case Inches:
            m = 1000. / 0.0254;
            break;
        }

        auto cos = std::cos(latitude * M_PI / 180.);
        auto cos2 = 2. * cos * cos - 1.;
        auto cos3 = 2. * cos * cos2 - cos;
        auto cos4 = 2. * cos * cos3 - cos2;
        auto cos5 = 2. * cos * cos4 - cos3;

        // multipliers for converting longitude and latitude
        // degrees into distance (http://1.usa.gov/1Wb1bv7)
        kx = m * (111.41513 * cos - 0.09455 * cos3 + 0.00012 * cos5);
        ky = m * (111.13209 - 0.56605 * cos2 + 0.0012 * cos4);
    }

    static CheapRuler fromTile(uint32_t y, uint32_t z) {
        double n = M_PI * (1. - 2. * (y + 0.5) / std::pow(2., z));
        double latitude = std::atan(0.5 * (std::exp(n) - std::exp(-n))) * 180. / M_PI;

        return CheapRuler(latitude);
    }

    //
    // Given two points of the form [x = longitude, y = latitude], returns the distance.
    //
    double distance(point a, point b) {
        auto dx = (a.x - b.x) * kx;
        auto dy = (a.y - b.y) * ky;

        return std::sqrt(dx * dx + dy * dy);
    }

    //
    // Returns the bearing between two points in angles.
    //
    double bearing(point a, point b) {
        auto dx = (b.x - a.x) * kx;
        auto dy = (b.y - a.y) * ky;

        if (!dx && !dy) {
            return 0.;
        }

        auto value = std::atan2(dx, dy) * 180. / M_PI;

        if (value > 180.) {
            value -= 360.;
        }

        return value;
    }

    //
    // Returns a new point given distance and bearing from the starting point.
    //
    point destination(point origin, double dist, double bearing_) {
        auto a = (90. - bearing_) * M_PI / 180.;

        return offset(origin, std::cos(a) * dist, std::sin(a) * dist);
    }

    //
    // Returns a new point given easting and northing offsets from the starting point.
    //
    point offset(point origin, double dx, double dy) {
        return point(origin.x + dx / kx, origin.y + dy / ky);
    }

    //
    // Given a line (an array of points), returns the total line distance.
    //
    double lineDistance(const line_string& points) {
        double total = 0.;

        for (unsigned i = 0; i < points.size() - 1; ++i) {
            total += distance(points[i], points[i + 1]);
        }

        return total;
    }

    //
    // Given a polygon (an array of rings, where each ring is an array of points),
    // returns the area.
    //
    double area(polygon poly) {
        double sum = 0.;

        for (unsigned i = 0; i < poly.size(); ++i) {
            auto& ring = poly[i];

            for (unsigned j = 0, len = ring.size(), k = len - 1; j < len; k = j++) {
                sum += (ring[j].x - ring[k].x) * (ring[j].y + ring[k].y) * (i ? -1. : 1.);
            }
        }

        return (std::abs(sum) / 2.) * kx * ky;
    }

    //
    // Returns the point at a specified distance along the line.
    //
    point along(const line_string& line, double dist) {
        double sum = 0.;

        if (dist <= 0.) {
            return line[0];
        }

        for (unsigned i = 0; i < line.size() - 1; ++i) {
            auto p0 = line[i];
            auto p1 = line[i + 1];
            auto d = distance(p0, p1);

            sum += d;

            if (sum > dist) {
                return interpolate(p0, p1, (dist - (sum - d)) / d);
            }
        }

        return line[line.size() - 1];
    }

    //
    // Returns a tuple of the form <point, index, t> where point is closest point on the line
    // from the given point, index is the start index of the segment with the closest point,
    // and t is a parameter from 0 to 1 that indicates where the closest point is on that segment.
    //
    std::tuple<point, unsigned, double> pointOnLine(const line_string& line, point p) {
        double minDist = std::numeric_limits<double>::infinity();
        double minX = 0., minY = 0., minI = 0., minT = 0.;

        for (unsigned i = 0; i < line.size() - 1; ++i) {
            auto t = 0.;
            auto x = line[i].x;
            auto y = line[i].y;
            auto dx = (line[i + 1].x - x) * kx;
            auto dy = (line[i + 1].y - y) * ky;

            if (dx != 0. || dy != 0.) {
                t = ((p.x - x) * kx * dx + (p.y - y) * ky * dy) / (dx * dx + dy * dy);

                if (t > 1) {
                    x = line[i + 1].x;
                    y = line[i + 1].y;

                } else if (t > 0) {
                    x += (dx / kx) * t;
                    y += (dy / ky) * t;
                }
            }

            dx = (p.x - x) * kx;
            dy = (p.y - y) * ky;

            auto sqDist = dx * dx + dy * dy;

            if (sqDist < minDist) {
                minDist = sqDist;
                minX = x;
                minY = y;
                minI = i;
                minT = t;
            }
        }

        return std::make_tuple(
                point(minX, minY), minI, ::fmax(0., ::fmin(1., minT)));
    }

    //
    // Returns a part of the given line between the start and the stop points (or their closest
    // points on the line).
    //
    line_string lineSlice(point start, point stop, const line_string& line) {
        auto getPoint = [](auto tuple) { return std::get<0>(tuple); };
        auto getIndex = [](auto tuple) { return std::get<1>(tuple); };
        auto getT     = [](auto tuple) { return std::get<2>(tuple); };

        auto p1 = pointOnLine(line, start);
        auto p2 = pointOnLine(line, stop);

        if (getIndex(p1) > getIndex(p2) || (getIndex(p1) == getIndex(p2) && getT(p1) > getT(p2))) {
            auto tmp = p1;
            p1 = p2;
            p2 = tmp;
        }

        line_string slice = { getPoint(p1) };

        auto l = getIndex(p1) + 1;
        auto r = getIndex(p2);

        if (line[l] != slice[0] && l <= r) {
            slice.push_back(line[l]);
        }

        for (unsigned i = l + 1; i <= r; ++i) {
            slice.push_back(line[i]);
        }

        if (line[r] != getPoint(p2)) {
            slice.push_back(getPoint(p2));
        }

        return slice;
    };

    //
    // Returns a part of the given line between the start and the stop points
    // indicated by distance along the line.
    //
    line_string lineSliceAlong(double start, double stop, const line_string& line) {
        double sum = 0.;
        line_string slice;

        for (unsigned i = 0; i < line.size() - 1; ++i) {
            auto p0 = line[i];
            auto p1 = line[i + 1];
            auto d = distance(p0, p1);

            sum += d;

            if (sum > start && slice.size() == 0) {
                slice.push_back(interpolate(p0, p1, (start - (sum - d)) / d));
            }

            if (sum >= stop) {
                slice.push_back(interpolate(p0, p1, (stop - (sum - d)) / d));
                return slice;
            }

            if (sum > start) {
                slice.push_back(p1);
            }
        }

        return slice;
    };

    //
    // Given a point, returns a bounding box object ([w, s, e, n])
    // created from the given point buffered by a given distance.
    //
    box bufferPoint(point p, double buffer) {
        auto v = buffer / ky;
        auto h = buffer / kx;

        return box(
            point(p.x - h, p.y - v),
            point(p.x + h, p.y + v)
        );
    }

    //
    // Given a bounding box, returns the box buffered by a given distance.
    //
    box bufferBBox(box bbox, double buffer) {
        auto v = buffer / ky;
        auto h = buffer / kx;

        return box(
            point(bbox.min.x - h, bbox.min.y - v),
            point(bbox.max.x + h, bbox.max.y + v)
        );
    }

    //
    // Returns true if the given point is inside in the given bounding box, otherwise false.
    //
    bool insideBBox(point p, box bbox) {
        return p.x >= bbox.min.x &&
               p.x <= bbox.max.x &&
               p.y >= bbox.min.y &&
               p.y <= bbox.max.y;
    }

    static point interpolate(point a, point b, double t) {
        double dx = b.x - a.x;
        double dy = b.y - a.y;

        return point(a.x + dx * t, a.y + dy * t);
    }

private:
    double ky;
    double kx;
};

} // namespace cheap_ruler
} // namespace mapbox
