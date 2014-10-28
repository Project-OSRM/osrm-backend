#ifndef SCHEMATIZED_PLANE_H_
#define SCHEMATIZED_PLANE_H_

#include "../Util/MercatorUtil.h"
#include "../Util/TrigonometryTables.h"
#include "SymbolicCoordinate.h"
#include "SubPath.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>

/*
 * Implements methods to operate on a d-schematized plane.
 */
class SchematizedPlane
{
public:
    // d_x = x0 + xy * d_y
    // d_y = yl * l
    struct EdgeTransform
    {
        double x0;
        double xy;
        double xl;
        double yl;
    };

    SchematizedPlane(unsigned d, double min_length)
    : d(d)
    , min_length(min_length)
    {
        // d needs to be even to we always have an angle of 45°
        BOOST_ASSERT(d % 2 == 0);

        for (unsigned i = 0; i < 4*d; i++)
        {
            double angle = i * 90.0/d * M_PI/180.0;
            allowed_angles.push_back(angle);

            EdgeTransform t;
            if (isVertical(i))
            {
                t = EdgeTransform {0.0, 0.0, cos(angle), sin(angle)};
            }
            else if(isHorizontal(i))
            {
                // account for edge in negative direction
                double sign = i == 0 ? 1.0 : -1.0;
                t = EdgeTransform {sign*min_length, 0.0, cos(angle), sin(angle)};
            }
            else
            {
                t = EdgeTransform {0.0, 1.0 / static_cast<double>(tan(angle)), cos(angle), sin(angle)};
            }
            angle_edge_transform.push_back(t);
        }
    }

    inline unsigned nextAngleAntiClockwise(unsigned angle_idx) const
    {
        return (angle_idx + 1) % (4*d);
    }

    inline unsigned nextAngleClockwise(unsigned angle_idx) const
    {
        // d - 1 = -1 (mod d), so we do not need signed intergers
        return (angle_idx + (4*d - 1)) % (4*d);
    }

    inline bool isVertical(unsigned angle_idx) const
    {
        return (angle_idx == d || angle_idx == 3*d);
    }

    inline bool isHorizontal(unsigned angle_idx) const
    {
        return (angle_idx == 2*d || angle_idx == 0);
    }

    /**
     * Returns the smallest number of angles between to given angles.
     */
    inline unsigned getAngleDiff(unsigned first_angle_idx, unsigned second_angle_idx) const
    {
        BOOST_ASSERT(first_angle_idx < std::numeric_limits<int>::max());
        BOOST_ASSERT(second_angle_idx < std::numeric_limits<int>::max());
        BOOST_ASSERT(first_angle_idx < 4*d);
        BOOST_ASSERT(second_angle_idx < 4*d);

        int diff = static_cast<int>(first_angle_idx) - static_cast<int>(second_angle_idx);
        if (diff < 0)
        {
            diff += 4*d;
        }

        return static_cast<unsigned>(diff);
    }

    /**
     * Returns true of the two angles are perpendicular.
     */
    inline bool isPerpendicular(unsigned first_angle_idx, unsigned second_angle_idx) const
    {
        unsigned diff = getAngleDiff(first_angle_idx, second_angle_idx);

        return (diff == d/2 || diff == 7*d/2);
    }

    /**
     * Returns true if the egdes with the given angles point in inverse directions, e.g. 0° and 180°.
     */
    inline bool isReversed(unsigned first_angle_idx, unsigned second_angle_idx) const
    {
        unsigned diff = getAngleDiff(first_angle_idx, second_angle_idx);

        return (diff == 2*d);
    }

    inline unsigned getPreferedDirection(double angle) const
    {
        unsigned prefered, alternative;
        getPreferedDirections(angle, prefered, alternative);

        return prefered;
    }

    /**
     * Computes the prefered (the angles that is the closest) and
     * the alternative (the angle that is second closest) of the given angle.
     */
    inline void getPreferedDirections(double angle, unsigned& prefered, unsigned& alternative) const
    {
        const unsigned lower = static_cast<unsigned>(d * angle/M_PI_2);
        const unsigned upper = (lower + 1) % allowed_angles.size();
        double diff_lower = std::fabs(allowed_angles[lower] - angle);
        double diff_upper = std::fabs(allowed_angles[upper] - angle);
        if (diff_lower > M_PI)
        {
            diff_lower = 2*M_PI - diff_lower;
        }
        if (diff_upper > M_PI)
        {
            diff_upper = 2*M_PI - diff_upper;
        }
        if( diff_lower > diff_upper)
        {
            prefered = upper;
            alternative = lower;
        }
        else
        {
            prefered = lower;
            alternative = upper;
        }
    }

    inline unsigned getPreferedDirection(const SymbolicCoordinate& a, const SymbolicCoordinate& b) const
    {
        const double angle = getAngle(a, b);
        return getPreferedDirection(angle);
    }

    /**
     * Returns dy to achieve min_length.
     * If the edge already has min_length, it returns 0.
     */
    inline double extendToMinLenEdge(unsigned direction, double edge_dy) const
    {
        double edge_dx = getAngleXOffset(direction, edge_dy);
        // TODO dy/sin(angle) = current_length <--- can be precalculated!
        double current_length = sqrt(edge_dx * edge_dx + edge_dy * edge_dy);
        double delta_length = min_length - current_length;

        // edge already has min_length or is horizontal
        if (delta_length <= 0.0 || isHorizontal(direction))
        {
            return 0;
        }

        double new_y = edge_dy + angle_edge_transform[direction].yl * delta_length;
        return new_y;
    }

    /**
     * Attaches coordinate system at A and computes angle of line segment to x-axis.
     */
    inline static double getAngle(const SymbolicCoordinate &A, const SymbolicCoordinate &B)
    {
        const double dx = B.x - A.x;
        const double dy = B.y - A.y;

        double angle = atan2_lookup(dy, dx);
        while (angle < 0)
        {
            angle += 2*M_PI;
        }
        return angle;
    }

    /**
     * Computes the x-offset that is necessary for to get the specified angle
     * with y-offset dy.
     */
    inline double getAngleXOffset(unsigned angle, double dy) const
    {
        return angle_edge_transform[angle].x0 + angle_edge_transform[angle].xy * dy;
    }

    /**
     * Computes dx and dy to achieve length with the given angle.
     */
    inline void getAngleOffsets(unsigned angle, double length, double& dx, double& dy) const
    {
        dx = angle_edge_transform[angle].xl * length;
        dy = angle_edge_transform[angle].yl * length;
    }

    /**
     * Mirror the line described by the direction on the axis describted by the first direction.
     */
    inline unsigned mirrorDirection(unsigned axis, unsigned direction) const
    {
        unsigned diff = getAngleDiff(direction, axis);
        return (axis + (d*4 - diff)) % (d*4);
    }

    /**
     * Transforms an angle that was computed on a x-monotone increasing path
     * to an angle in the given monoticity.
     *
     * We need this to map angles we computed while schematizing a (transformed)
     * subpath to angles after the subpath was transformed to its original monoticity.
     */
    inline unsigned fromXMonotoneDirection(unsigned angle, PathMonotiticy monoticity) const
    {
        if (monoticity & MONOTONE_INCREASING_X)
        {
            return angle;
        }

        if (monoticity & MONOTONE_INCREASING_Y)
        {
            // mirror on axis going through (0, 0) and (1, 1) which always maps to d/2
            return mirrorDirection(d/2, angle);
        }

        if (monoticity & MONOTONE_DECREASING_Y)
        {
            unsigned yinc_angle = mirrorDirection(d, angle);
            return mirrorDirection(d/2, yinc_angle);
        }

        if (monoticity & MONOTONE_DECREASING_X)
        {
            // mirror on y axis which always maps to d
            return mirrorDirection(d, angle);
        }

        BOOST_ASSERT(monoticity != MONOTONE_INVALID);
        return std::numeric_limits<unsigned>::max();
    }


private:
    std::vector<float> allowed_angles;
    std::vector<EdgeTransform> angle_edge_transform;
    unsigned d;
    double min_length;
};

#endif
