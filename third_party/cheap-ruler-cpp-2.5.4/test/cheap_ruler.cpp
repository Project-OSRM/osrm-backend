#include <mapbox/cheap_ruler.hpp>
#include <gtest/gtest.h>

#include "fixtures/lines.hpp"
#include "fixtures/turf.hpp"

namespace cr = mapbox::cheap_ruler;

class CheapRulerTest : public ::testing::Test {
protected:
    cr::CheapRuler ruler = cr::CheapRuler(32.8351);
    cr::CheapRuler milesRuler = cr::CheapRuler(32.8351, cr::CheapRuler::Miles);
};

void assertErr(double expected, double actual, double maxError) {
    // Add a negligible fraction to make sure we
    // don't divide by zero.
    double error = std::abs((actual - expected) /
            (expected == 0. ? expected + 0.000001 : expected));

    if (error > maxError) {
        FAIL() << "expected is " << expected << " but got " << actual;
    }
}

TEST_F(CheapRulerTest, distance) {
    for (unsigned i = 0; i < points.size() - 1; ++i) {
        auto expected = turf_distance[i];
        auto actual = ruler.distance(points[i], points[i + 1]);

        assertErr(expected, actual, .003);
    }
}

TEST_F(CheapRulerTest, distanceInMiles) {
    auto d = ruler.distance({ 30.5, 32.8351 }, { 30.51, 32.8451 });
    auto d2 = milesRuler.distance({ 30.5, 32.8351 }, { 30.51, 32.8451 });

    assertErr(d / d2, 1.609344, 1e-12);
}

TEST_F(CheapRulerTest, bearing) {
    for (unsigned i = 0; i < points.size() - 1; ++i) {
        auto expected = turf_bearing[i];
        auto actual = ruler.bearing(points[i], points[i + 1]);

        assertErr(expected, actual, .005);
    }
}

TEST_F(CheapRulerTest, destination) {
    for (unsigned i = 0; i < points.size(); ++i) {
        auto bearing = (i % 360) - 180.;
        auto expected = turf_destination[i];
        auto actual = ruler.destination(points[i], 1.0, bearing);

        assertErr(expected.x, actual.x, 1e-6); // longitude
        assertErr(expected.y, actual.y, 1e-6); // latitude
    }
}

TEST_F(CheapRulerTest, lineDistance) {
    for (unsigned i = 0; i < lines.size(); ++i) {
        auto expected = turf_lineDistance[i];
        auto actual = ruler.lineDistance(lines[i]);

        assertErr(expected, actual, 0.003);
    }
}

TEST_F(CheapRulerTest, area) {
    for (unsigned i = 0, j = 0; i < lines.size(); ++i) {
        if (lines[i].size() < 3) {
            continue;
        }

        cr::linear_ring ring;
        for (auto point : lines[i]) {
            ring.push_back(point);
        }
        ring.push_back(lines[i][0]);

        auto expected = turf_area[j++];
        auto actual = ruler.area(cr::polygon{ ring });

        assertErr(expected, actual, 0.003);
    }
}

TEST_F(CheapRulerTest, along) {
    for (unsigned i = 0; i < lines.size(); ++i) {
        auto expected = turf_along[i];
        auto actual = ruler.along(lines[i], turf_along_dist[i]);

        assertErr(expected.x, actual.x, 1e-6); // along longitude
        assertErr(expected.y, actual.y, 1e-6); // along latitude
    }
}

TEST_F(CheapRulerTest, alongWithDist) {
    ASSERT_EQ(ruler.along(lines[0], -5), lines[0][0]);
}

TEST_F(CheapRulerTest, alongWithDistGreaterThanLength) {
    ASSERT_EQ(ruler.along(lines[0], 1000), lines[0][lines[0].size() - 1]);
}

TEST_F(CheapRulerTest, pointOnLine) {
    // not Turf comparison because pointOnLine is bugged https://github.com/Turfjs/turf/issues/344
    cr::line_string line = {{ -77.031669, 38.878605 }, { -77.029609, 38.881946 }};
    auto result = ruler.pointOnLine(line, { -77.034076, 38.882017 });

    ASSERT_EQ(std::get<0>(result), cr::point(-77.03052697027461, 38.880457194811896)); // point
    ASSERT_EQ(std::get<1>(result), 0u); // index
    ASSERT_EQ(std::get<2>(result), 0.5543833618360235); // t

    ASSERT_EQ(std::get<2>(ruler.pointOnLine(line, { -80., 38. })), 0.) << "t is not less than 0";
    ASSERT_EQ(std::get<2>(ruler.pointOnLine(line, { -75., 38. })), 1.) << "t is not bigger than 1";
}

TEST_F(CheapRulerTest, lineSlice) {
    for (unsigned i = 0; i < lines.size(); ++i) {
        auto line = lines[i];
        auto dist = ruler.lineDistance(line);
        auto start = ruler.along(line, dist * 0.3);
        auto stop = ruler.along(line, dist * 0.7);
        auto expected = turf_lineSlice[i];
        auto actual = ruler.lineDistance(ruler.lineSlice(start, stop, line));

        assertErr(expected, actual, 1e-5);
    }
}

TEST_F(CheapRulerTest, lineSliceAlong) {
    for (unsigned i = 0; i < lines.size(); ++i) {
        if (i == 46) {
            // skip due to Turf bug https://github.com/Turfjs/turf/issues/351
            continue;
        };

        auto line = lines[i];
        auto dist = ruler.lineDistance(line);
        auto expected = turf_lineSlice[i];
        auto actual = ruler.lineDistance(ruler.lineSliceAlong(dist * 0.3, dist * 0.7, line));

        assertErr(expected, actual, 1e-5);
    }
}

TEST_F(CheapRulerTest, lineSliceReverse) {
    auto line = lines[0];
    auto dist = ruler.lineDistance(line);
    auto start = ruler.along(line, dist * 0.7);
    auto stop = ruler.along(line, dist * 0.3);
    auto actual = ruler.lineDistance(ruler.lineSlice(start, stop, line));

    ASSERT_EQ(actual, 0.018676802802910702);
}

TEST_F(CheapRulerTest, bufferPoint) {
    for (unsigned i = 0; i < points.size(); ++i) {
        auto expected = turf_bufferPoint[i];
        auto actual = milesRuler.bufferPoint(points[i], 0.1);

        assertErr(expected.min.x, actual.min.x, 2e-7);
        assertErr(expected.min.x, actual.min.x, 2e-7);
        assertErr(expected.max.y, actual.max.y, 2e-7);
        assertErr(expected.max.y, actual.max.y, 2e-7);
    }
}

TEST_F(CheapRulerTest, bufferBBox) {
    cr::box bbox({ 30, 38 }, { 40, 39 });
    cr::box bbox2 = ruler.bufferBBox(bbox, 1);

    ASSERT_EQ(bbox2, cr::box({ 29.989319515875376, 37.99098271225711 }, { 40.01068048412462, 39.00901728774289 }));
}

TEST_F(CheapRulerTest, insideBBox) {
    cr::box bbox({ 30, 38 }, { 40, 39 });

    ASSERT_TRUE(ruler.insideBBox({ 35, 38.5 }, bbox));
    ASSERT_FALSE(ruler.insideBBox({ 45, 45 }, bbox));
}

TEST_F(CheapRulerTest, fromTile) {
    auto ruler1 = cr::CheapRuler(50.5);
    auto ruler2 = cr::CheapRuler::fromTile(11041, 15);

    cr::point p1(30.5, 50.5);
    cr::point p2(30.51, 50.51);

    assertErr(ruler1.distance(p1, p2), ruler2.distance(p1, p2), 2e-5);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
