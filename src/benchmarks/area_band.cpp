/*
 * What does it cost to smooth a plaza path?
 *
 * The elastic band (engine/area_band.hpp) rounds the corners of a taut path and holds it
 * off the geometry by a comfort margin.  The plan's budget for the whole area answer is
 * about a millisecond, and the band has to be measured against that before it is switched
 * on anywhere: a /route pays it once per leg, a /table would pay it per cell.
 *
 * This times smooth() over taut paths threading a grid of square obstacles, at the
 * comfort margins a profile would set, and beside it round_corners(), the straight-lines-
 * and-arcs construction that replaced the band as what the engine draws.  Units are
 * metres throughout, which is what both see after metres_per_projected_unit().
 *
 * The paths are 1.25 km long, far longer than any plaza leg, so that the per-node cost
 * shows and the totals are read as rates rather than as what a request pays: about 1.7
 * microseconds per node per ring vertex, measured 2026-09-04.  A row whose node count is
 * the taut path's own is one the reversal gate declined after paying the full cost, which
 * happens at the finest margins here because five thousand nodes of sub-degree jitter sum
 * to more than the gate's slack; plaza-length paths do not reach that.
 */
#include "engine/area_band.hpp"
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

using osrm::engine::area::BandParameters;
using osrm::engine::area::Point;
using osrm::engine::area::Ring;
using osrm::engine::area::smooth;

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
    std::cout << "smooth() over a taut path threading a grid of obstacles, per band\n\n"
              << std::setw(8) << "vertices" << std::setw(7) << "bends" << std::setw(9)
              << "margin" << std::setw(10) << "nodes" << std::setw(11) << "certified"
              << std::setw(10) << "length" << std::setw(12) << "microsec" << std::setw(14)
              << "arcs microsec" << '\n';

    for (const std::size_t per_side : {1, 2, 3, 4, 6})
    {
        const Plaza plaza(per_side);
        for (const double margin : {1.0, 2.0, 5.0})
        {
            BandParameters parameters;
            parameters.comfort = margin;

            // enough runs that the number is the band's and not the clock's
            constexpr int RUNS = 20;
            auto nodes = std::size_t{0};
            auto certified = true;
            auto ratio = 0.0;
            const auto started = std::chrono::steady_clock::now();
            for (int run = 0; run < RUNS; ++run)
            {
                const auto band = smooth(plaza.taut, plaza.rings, parameters);
                nodes = band.points.size();
                certified = band.certified;
                ratio = length(band.points) / length(plaza.taut);
            }
            const auto elapsed = std::chrono::duration<double, std::micro>(
                                     std::chrono::steady_clock::now() - started)
                                     .count() /
                                 RUNS;

            const auto started_arcs = std::chrono::steady_clock::now();
            for (int run = 0; run < RUNS; ++run)
            {
                const auto rounded = round_corners(plaza.taut, plaza.rings, margin);
                (void)rounded;
            }
            const auto elapsed_arcs = std::chrono::duration<double, std::micro>(
                                          std::chrono::steady_clock::now() - started_arcs)
                                          .count() /
                                      RUNS;

            std::cout << std::setw(8) << plaza.vertices() << std::setw(7)
                      << plaza.taut.size() - 2 << std::setw(8) << std::fixed
                      << std::setprecision(0) << margin << "m" << std::setw(10) << nodes
                      << std::setw(11) << (certified ? "yes" : "no") << std::setw(9)
                      << std::setprecision(3) << ratio << "x" << std::setw(12)
                      << std::setprecision(0) << elapsed << std::setw(14) << elapsed_arcs
                      << '\n';
        }
    }
    return 0;
}
