#include <mapbox/geometry.hpp>

#include <cassert>

using namespace mapbox::geometry;

static void testPoint() {
    point<double> p1;
    assert(int(p1.x) == 0);
    assert(int(p1.y) == 0);

    point<uint32_t> p2(2, 3);
    point<uint32_t> p3(4, 6);

    assert((p2 + p3) == point<uint32_t>(6, 9));
    assert((p2 + 1u) == point<uint32_t>(3, 4));
    assert((p3 - p2) == point<uint32_t>(2, 3));
    assert((p3 - 1u) == point<uint32_t>(3, 5));
    assert((p3 * p2) == point<uint32_t>(8, 18));
    assert((p2 * 2u) == point<uint32_t>(4, 6));
    assert((p3 / p2) == point<uint32_t>(2, 2));
    assert((p3 / 2u) == point<uint32_t>(2, 3));

    { point<uint32_t> p(2, 3); assert((p += p3) == point<uint32_t>(6, 9)); }
    { point<uint32_t> p(2, 3); assert((p += 1u) == point<uint32_t>(3, 4)); }
    { point<uint32_t> p(4, 6); assert((p -= p2) == point<uint32_t>(2, 3)); }
    { point<uint32_t> p(4, 6); assert((p -= 1u) == point<uint32_t>(3, 5)); }
    { point<uint32_t> p(4, 6); assert((p *= p2) == point<uint32_t>(8, 18)); }
    { point<uint32_t> p(2, 3); assert((p *= 2u) == point<uint32_t>(4, 6)); }
    { point<uint32_t> p(4, 6); assert((p /= p2) == point<uint32_t>(2, 2)); }
    { point<uint32_t> p(4, 6); assert((p /= 2u) == point<uint32_t>(2, 3)); }
}

static void testMultiPoint() {
    multi_point<double> mp1;
    assert(mp1.size() == 0);

    multi_point<double> mp2(10);
    assert(mp2.size() == 10);

    assert(mp1 == mp1);
    assert(!(mp1 != mp1));
    assert(mp1 != mp2);
}

static void testLineString() {
    line_string<double> ls1;
    assert(ls1.size() == 0);

    line_string<double> ls2(10);
    assert(ls2.size() == 10);

    assert(ls1 == ls1);
    assert(!(ls1 != ls1));
    assert(ls1 != ls2);
}

static void testMultiLineString() {
    multi_line_string<double> mls1;
    assert(mls1.size() == 0);

    multi_line_string<double> mls2(10);
    assert(mls2.size() == 10);

    assert(mls1 == mls1);
    assert(!(mls1 != mls1));
    assert(mls1 != mls2);
}

static void testPolygon() {
    polygon<double> pg1;
    assert(pg1.size() == 0);

    polygon<double> pg2({{{0, 1}}});
    assert(pg2.size() == 1);
    assert(pg2[0].size() == 1);
    assert(pg2[0][0] == point<double>(0, 1));

    assert(pg1 == pg1);
    assert(!(pg1 != pg1));
    assert(pg1 != pg2);
}

static void testMultiPolygon() {
    multi_polygon<double> mpg1;
    assert(mpg1.size() == 0);

    multi_polygon<double> mpg2(10);
    assert(mpg2.size() == 10);

    assert(mpg1 == mpg1);
    assert(!(mpg1 != mpg1));
    assert(mpg1 != mpg2);
}

static void testGeometry() {
    geometry<double> pg { point<double>() };
    assert(pg.is<point<double>>());

    geometry<double> lsg { line_string<double>() };
    assert(lsg.is<line_string<double>>());

    geometry<double> pgg { polygon<double>() };
    assert(pgg.is<polygon<double>>());

    geometry<double> mpg { multi_point<double>() };
    assert(mpg.is<multi_point<double>>());

    geometry<double> mlsg { multi_line_string<double>() };
    assert(mlsg.is<multi_line_string<double>>());

    geometry<double> mpgg { multi_polygon<double>() };
    assert(mpgg.is<multi_polygon<double>>());

    geometry<double> gcg { geometry_collection<double>() };
    assert(gcg.is<geometry_collection<double>>());

    assert(pg == pg);
    assert(!(pg != pg));
    assert(pg != lsg);
}

static void testGeometryCollection() {
    geometry_collection<double> gc1;
    assert(gc1.size() == 0);

    assert(gc1 == gc1);
    assert(!(gc1 != gc1));
}

static void testFeature() {
    feature<double> pf { point<double>() };
    assert(pf.geometry.is<point<double>>());
    assert(pf.properties.size() == 0);

    auto &p = pf.properties;

    p["bool"] = true;
    p["string"] = std::string("foo");
    p["double"] = 2.5;
    p["uint"] = uint64_t(10);
    p["int"] = int64_t(-10);
    p["null"] = null_value;

    assert(p["bool"].is<bool>());
    assert(p["bool"] == true);
    assert(p["string"].is<std::string>());
    assert(p["string"] == std::string("foo"));
    assert(p["double"].is<double>());
    assert(p["double"] == 2.5);
    assert(p["uint"].is<uint64_t>());
    assert(p["uint"] == uint64_t(10));
    assert(p["int"].is<int64_t>());
    assert(p["int"] == int64_t(-10));
    assert(p["null"].is<null_value_t>());
    assert(p["null"] == null_value);

    p["null"] = null_value_t{};
    assert(p["null"].is<null_value_t>());
    assert(p["null"] == null_value);

    assert(p == p);
    assert(!(p != p));

    assert(pf == pf);
    assert(!(pf != pf));

    assert(p.size() == 6);

    feature<double> id1 { point<double>() };
    id1.id = { uint64_t(1) };

    feature<double> id2 { point<double>() };
    id1.id = { uint64_t(2) };

    assert(id1 == id1);
    assert(id1 != id2);
}

static void testFeatureCollection() {
    feature_collection<double> fc1;
    assert(fc1.size() == 0);

    assert(fc1 == fc1);
    assert(!(fc1 != fc1));
}

struct point_counter {
    std::size_t count = 0;
    template <class Point>
    void operator()(Point const&) { count++; };
};

static void testForEachPoint() {
    auto count_points = [] (auto const& g) {
        point_counter counter;
        for_each_point(g, counter);
        return counter.count;
    };

    assert(count_points(point<double>()) == 1);
    assert(count_points(line_string<double>({{0, 1}, {2, 3}})) == 2);
    assert(count_points(geometry<double>(polygon<double>({{{0, 1}, {2, 3}}}))) == 2);

    auto point_negator = [] (point<double>& p) { p *= -1.0; };

    point<double> p(1, 2);
    for_each_point(p, point_negator);
    assert(p == point<double>(-1, -2));

    line_string<double> ls({{0, 1}, {2, 3}});
    for_each_point(ls, point_negator);
    assert(ls == line_string<double>({{0, -1}, {-2, -3}}));

    geometry<double> g(polygon<double>({{{0, 1}, {2, 3}}}));
    for_each_point(g, point_negator);
    assert(g == geometry<double>(polygon<double>({{{0, -1}, {-2, -3}}})));

    // Custom geometry type
    using my_geometry = mapbox::util::variant<point<double>>;
    assert(count_points(my_geometry(point<double>())) == 1);

    // Custom point type
    struct my_point {
        int16_t x;
        int16_t y;
    };
    assert(count_points(std::vector<my_point>({my_point{0, 1}})) == 1);
    assert(count_points(mapbox::util::variant<my_point>(my_point{0, 1})) == 1);
}

static void testEnvelope() {
    assert(envelope(point<double>(0, 0)) == box<double>({0, 0}, {0, 0}));
    assert(envelope(line_string<double>({{0, 1}, {2, 3}})) == box<double>({0, 1}, {2, 3}));
    assert(envelope(polygon<double>({{{0, 1}, {2, 3}}})) == box<double>({0, 1}, {2, 3}));

    assert(envelope(multi_point<double>({{0, 0}})) == box<double>({0, 0}, {0, 0}));
    assert(envelope(multi_line_string<double>({{{0, 1}, {2, 3}}})) == box<double>({0, 1}, {2, 3}));
    assert(envelope(multi_polygon<double>({{{{0, 1}, {2, 3}}}})) == box<double>({0, 1}, {2, 3}));

    assert(envelope(geometry<int>(point<int>(0, 0))) == box<int>({0, 0}, {0, 0}));
    assert(envelope(geometry_collection<int>({point<int>(0, 0)})) == box<int>({0, 0}, {0, 0}));
}

int main() {
    testPoint();
    testMultiPoint();
    testLineString();
    testMultiLineString();
    testPolygon();
    testMultiPolygon();
    testGeometry();
    testGeometryCollection();
    testFeature();
    testFeatureCollection();

    testForEachPoint();
    testEnvelope();
    return 0;
}
