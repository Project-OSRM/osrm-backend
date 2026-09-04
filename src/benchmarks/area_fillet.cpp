/*
 * What does it cost to round the corners of a plaza path?
 *
 * round_corners() (engine/area_fillet.hpp) redraws a taut path as straight lines and
 * tangent arcs held a margin off the geometry.  A /route pays it once per leg across an
 * area; a /table would pay it per cell, which is why it is measured.
 *
 * This times it over taut paths threading a grid of square obstacles, at the margins a
 * profile would set.  Units are metres throughout, which is what it sees after
 * metres_per_projected_unit() has been applied.  The paths are 1.25 km long, far longer
 * than any plaza leg, so the totals read as rates.
 */
#include "engine/area_fillet.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <span>
#include <vector>

namespace
{

using osrm::engine::area::Point;
using osrm::engine::area::Ring;
using osrm::engine::area::round_corners;

/** A 1 km square plaza with a grid of 45 m blocks in it, as area_geodesic.cpp lays out. */
struct Plaza
{
    std::vector<Point> outer;
    std::vector<std::vector<Point>> blocks;
    std::vector<Ring> rings;
    std::vector<Point> taut;

    explicit Plaza(const std::size_t per_side)
    {
        const double side = 1000.0;
        outer = {{0, 0}, {side, 0}, {side, side}, {0, side}};
        const double step = per_side > 1 ? 760.0 / static_cast<double>(per_side - 1) : 0.0;
        for (std::size_t i = 0; i < per_side; ++i)
        {
            for (std::size_t j = 0; j < per_side; ++j)
            {
                const double x = 120.0 + static_cast<double>(i) * step;
                const double y = 120.0 + static_cast<double>(j) * step;
                blocks.push_back({{x, y}, {x + 45, y}, {x + 45, y + 45}, {x, y + 45}});
            }
        }
        rings.emplace_back(outer);
        for (const auto &block : blocks)
        {
            rings.emplace_back(block);
        }

        // The taut path along the diagonal, turning on the north-west corner of each
        // block it passes.  Every segment leaves a corner at 45 degrees and so grazes the
        // block without entering it, which is the shape a real shortest path has.
        taut.push_back({60.0, 60.0});
        for (std::size_t i = 0; i < per_side; ++i)
        {
            const double x = 120.0 + static_cast<double>(i) * step;
            taut.push_back({x, x + 45.0});
        }
        taut.push_back({940.0, 940.0});
    }

    std::size_t vertices() const { return 4 + 4 * blocks.size(); }
};

double length(std::span<const Point> points)
{
    auto total = 0.0;
    for (std::size_t i = 1; i < points.size(); ++i)
    {
        total += std::hypot(points[i].x - points[i - 1].x, points[i].y - points[i - 1].y);
    }
    return total;
}

} // namespace

int main()
{
    std::cout << "round_corners() over a taut path threading a grid of obstacles, per path\n\n"
              << std::setw(8) << "vertices" << std::setw(7) << "bends" << std::setw(9)
              << "margin" << std::setw(10) << "points" << std::setw(8) << "legal"
              << std::setw(10) << "length" << std::setw(12) << "microsec" << '\n';

    for (const std::size_t per_side : {1, 2, 3, 4, 6})
    {
        const Plaza plaza(per_side);
        for (const double margin : {1.0, 2.0, 5.0})
        {
            // enough runs that the number is the construction's and not the clock's
            constexpr int RUNS = 2000;
            auto points = std::size_t{0};
            auto legal = true;
            auto ratio = 0.0;
            const auto started = std::chrono::steady_clock::now();
            for (int run = 0; run < RUNS; ++run)
            {
                const auto rounded = round_corners(plaza.taut, plaza.rings, margin);
                points = rounded.points.size();
                legal = rounded.legal;
                ratio = length(rounded.points) / length(plaza.taut);
            }
            const auto elapsed = std::chrono::duration<double, std::micro>(
                                     std::chrono::steady_clock::now() - started)
                                     .count() /
                                 RUNS;

            std::cout << std::setw(8) << plaza.vertices() << std::setw(7)
                      << plaza.taut.size() - 2 << std::setw(8) << std::fixed
                      << std::setprecision(0) << margin << "m" << std::setw(10) << points
                      << std::setw(8) << (legal ? "yes" : "no") << std::setw(9)
                      << std::setprecision(3) << ratio << "x" << std::setw(12)
                      << std::setprecision(1) << elapsed << '\n';
        }
    }
    return 0;
}
