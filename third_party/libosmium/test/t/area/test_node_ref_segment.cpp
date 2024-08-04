#include "catch.hpp"

#include <osmium/area/detail/node_ref_segment.hpp>

using osmium::area::detail::NodeRefSegment;
using osmium::area::detail::role_type;

TEST_CASE("Default construction of NodeRefSegment") {
    const NodeRefSegment s;
    REQUIRE(s.first().ref() == 0);
    REQUIRE(s.first().location() == osmium::Location());
    REQUIRE(s.second().ref() == 0);
    REQUIRE(s.second().location() == osmium::Location());
}

TEST_CASE("Construction of NodeRefSegment with NodeRefs") {
    const osmium::NodeRef nr1{1, {1.2, 3.4}};
    const osmium::NodeRef nr2{2, {1.4, 3.1}};
    const osmium::NodeRef nr3{3, {1.2, 3.6}};
    const osmium::NodeRef nr4{4, {1.2, 3.7}};

    const NodeRefSegment s1{nr1, nr2, role_type::unknown, nullptr};
    REQUIRE(s1.first().ref() == 1);
    REQUIRE(s1.second().ref() == 2);

    const NodeRefSegment s2{nr2, nr3, role_type::unknown, nullptr};
    REQUIRE(s2.first().ref() == 3);
    REQUIRE(s2.second().ref() == 2);

    const NodeRefSegment s3{nr3, nr4, role_type::unknown, nullptr};
    REQUIRE(s3.first().ref() == 3);
    REQUIRE(s3.second().ref() == 4);
}

TEST_CASE("Intersection of NodeRefSegments") {
    const NodeRefSegment s1{{ 1, {0.0, 0.0}}, { 2, {2.0, 2.0}}, role_type::unknown, nullptr};
    const NodeRefSegment s2{{ 3, {0.0, 2.0}}, { 4, {2.0, 0.0}}, role_type::unknown, nullptr};
    const NodeRefSegment s3{{ 5, {2.0, 0.0}}, { 6, {4.0, 2.0}}, role_type::unknown, nullptr};
    const NodeRefSegment s4{{ 7, {1.0, 0.0}}, { 8, {3.0, 2.0}}, role_type::unknown, nullptr};
    const NodeRefSegment s5{{ 9, {0.0, 4.0}}, {10, {4.0, 0.0}}, role_type::unknown, nullptr};
    const NodeRefSegment s6{{11, {0.0, 0.0}}, {12, {1.0, 1.0}}, role_type::unknown, nullptr};
    const NodeRefSegment s7{{13, {1.0, 1.0}}, {14, {3.0, 3.0}}, role_type::unknown, nullptr};

    REQUIRE(calculate_intersection(s1, s2) == osmium::Location(1.0, 1.0));
    REQUIRE(calculate_intersection(s2, s1) == osmium::Location(1.0, 1.0));

    REQUIRE(calculate_intersection(s1, s3) == osmium::Location());
    REQUIRE(calculate_intersection(s3, s1) == osmium::Location());

    REQUIRE(calculate_intersection(s2, s3) == osmium::Location());
    REQUIRE(calculate_intersection(s3, s2) == osmium::Location());

    REQUIRE(calculate_intersection(s1, s4) == osmium::Location());
    REQUIRE(calculate_intersection(s4, s1) == osmium::Location());

    REQUIRE(calculate_intersection(s1, s5) == osmium::Location(2.0, 2.0));
    REQUIRE(calculate_intersection(s5, s1) == osmium::Location(2.0, 2.0));

    REQUIRE(calculate_intersection(s1, s6) == osmium::Location(1.0, 1.0));
    REQUIRE(calculate_intersection(s6, s1) == osmium::Location(1.0, 1.0));

    REQUIRE(calculate_intersection(s1, s7) == osmium::Location(1.0, 1.0));
    REQUIRE(calculate_intersection(s7, s1) == osmium::Location(1.0, 1.0));

    REQUIRE(calculate_intersection(s6, s7) == osmium::Location());
    REQUIRE(calculate_intersection(s7, s6) == osmium::Location());
}

TEST_CASE("Intersection of collinear NodeRefSegments") {
    const NodeRefSegment s1{{ 1, {0.0, 0.0}}, { 2, {2.0, 0.0}}, role_type::unknown, nullptr}; // *---*
    const NodeRefSegment s2{{ 3, {2.0, 0.0}}, { 4, {4.0, 0.0}}, role_type::unknown, nullptr}; //     *---*
    const NodeRefSegment s3{{ 5, {0.0, 0.0}}, { 6, {1.0, 0.0}}, role_type::unknown, nullptr}; // *-*
    const NodeRefSegment s4{{ 7, {1.0, 0.0}}, { 8, {2.0, 0.0}}, role_type::unknown, nullptr}; //   *-*
    const NodeRefSegment s5{{ 9, {1.0, 0.0}}, {10, {3.0, 0.0}}, role_type::unknown, nullptr}; //   *---*
    const NodeRefSegment s6{{11, {0.0, 0.0}}, {12, {4.0, 0.0}}, role_type::unknown, nullptr}; // *-------*
    const NodeRefSegment s7{{13, {0.0, 0.0}}, {14, {5.0, 0.0}}, role_type::unknown, nullptr}; // *---------*
    const NodeRefSegment s8{{13, {1.0, 0.0}}, {14, {5.0, 0.0}}, role_type::unknown, nullptr}; //   *-------*
    const NodeRefSegment s9{{13, {3.0, 0.0}}, {14, {4.0, 0.0}}, role_type::unknown, nullptr}; //       *-*

    REQUIRE(calculate_intersection(s1, s1) == osmium::Location());

    REQUIRE(calculate_intersection(s1, s2) == osmium::Location());
    REQUIRE(calculate_intersection(s2, s1) == osmium::Location());

    REQUIRE(calculate_intersection(s1, s3) == osmium::Location(1.0, 0.0));
    REQUIRE(calculate_intersection(s3, s1) == osmium::Location(1.0, 0.0));

    REQUIRE(calculate_intersection(s1, s4) == osmium::Location(1.0, 0.0));
    REQUIRE(calculate_intersection(s4, s1) == osmium::Location(1.0, 0.0));

    REQUIRE(calculate_intersection(s1, s5) == osmium::Location(1.0, 0.0));
    REQUIRE(calculate_intersection(s5, s1) == osmium::Location(1.0, 0.0));

    REQUIRE(calculate_intersection(s1, s6) == osmium::Location(2.0, 0.0));
    REQUIRE(calculate_intersection(s6, s1) == osmium::Location(2.0, 0.0));

    REQUIRE(calculate_intersection(s1, s7) == osmium::Location(2.0, 0.0));
    REQUIRE(calculate_intersection(s7, s1) == osmium::Location(2.0, 0.0));

    REQUIRE(calculate_intersection(s1, s8) == osmium::Location(1.0, 0.0));
    REQUIRE(calculate_intersection(s8, s1) == osmium::Location(1.0, 0.0));

    REQUIRE(calculate_intersection(s1, s9) == osmium::Location());
    REQUIRE(calculate_intersection(s9, s1) == osmium::Location());

    REQUIRE(calculate_intersection(s5, s6) == osmium::Location(1.0, 0.0));
    REQUIRE(calculate_intersection(s6, s5) == osmium::Location(1.0, 0.0));

    REQUIRE(calculate_intersection(s7, s8) == osmium::Location(1.0, 0.0));
    REQUIRE(calculate_intersection(s8, s7) == osmium::Location(1.0, 0.0));
}

TEST_CASE("Intersection of very long NodeRefSegments") {
    const NodeRefSegment s1{{1, {90.0, 90.0}}, {2, {-90.0, -90.0}}, role_type::unknown, nullptr};
    const NodeRefSegment s2{{1, {-90.0, 90.0}}, {2, {90.0, -90.0}}, role_type::unknown, nullptr};
    REQUIRE(calculate_intersection(s1, s2) == osmium::Location(0.0, 0.0));

    const NodeRefSegment s3{{1, {-90.0, -90.0}}, {2, {90.0, 90.0}}, role_type::unknown, nullptr};
    const NodeRefSegment s4{{1, {-90.0, 90.0}}, {2, {90.0, -90.0}}, role_type::unknown, nullptr};
    REQUIRE(calculate_intersection(s3, s4) == osmium::Location(0.0, 0.0));

    const NodeRefSegment s5{{1, {-90.00000001, -90.0}}, {2, {90.0, 90.0}}, role_type::unknown, nullptr};
    const NodeRefSegment s6{{1, {-90.0, 90.0}}, {2, {90.0, -90.0}}, role_type::unknown, nullptr};
    REQUIRE(calculate_intersection(s5, s6) == osmium::Location(0.0, 0.0));
}

TEST_CASE("Ordering of NodeRefSegements") {
    const osmium::NodeRef node_ref1{1, {1.0, 3.0}};
    const osmium::NodeRef node_ref2{2, {1.4, 2.9}};
    const osmium::NodeRef node_ref3{3, {1.2, 3.0}};
    const osmium::NodeRef node_ref4{4, {1.2, 3.3}};

    REQUIRE(node_ref1 < node_ref2);
    REQUIRE(node_ref2 < node_ref3);
    REQUIRE(node_ref1 < node_ref3);
    REQUIRE(node_ref1 >= node_ref1);

    REQUIRE(      osmium::location_less()(node_ref1, node_ref2));
    REQUIRE_FALSE(osmium::location_less()(node_ref2, node_ref3));
    REQUIRE(      osmium::location_less()(node_ref1, node_ref3));
    REQUIRE(      osmium::location_less()(node_ref3, node_ref4));
    REQUIRE_FALSE(osmium::location_less()(node_ref1, node_ref1));
}

TEST_CASE("More ordering of NodeRefSegments") {
    const osmium::NodeRef nr0{0, {0.0, 0.0}};
    const osmium::NodeRef nr1{1, {1.0, 0.0}};
    const osmium::NodeRef nr2{2, {0.0, 1.0}};
    const osmium::NodeRef nr3{3, {2.0, 0.0}};
    const osmium::NodeRef nr4{4, {0.0, 2.0}};
    const osmium::NodeRef nr5{5, {1.0, 1.0}};
    const osmium::NodeRef nr6{6, {2.0, 2.0}};
    const osmium::NodeRef nr7{6, {1.0, 2.0}};

    const NodeRefSegment s1{nr0, nr1, role_type::unknown, nullptr};
    const NodeRefSegment s2{nr0, nr2, role_type::unknown, nullptr};
    const NodeRefSegment s3{nr0, nr3, role_type::unknown, nullptr};
    const NodeRefSegment s4{nr0, nr4, role_type::unknown, nullptr};
    const NodeRefSegment s5{nr0, nr5, role_type::unknown, nullptr};
    const NodeRefSegment s6{nr0, nr6, role_type::unknown, nullptr};
    const NodeRefSegment s7{nr0, nr7, role_type::unknown, nullptr};

    // s1
    REQUIRE_FALSE(s1 < s1);
    REQUIRE(s2 < s1);
    REQUIRE(s1 < s3);
    REQUIRE(s4 < s1);
    REQUIRE(s5 < s1);
    REQUIRE(s6 < s1);
    REQUIRE(s7 < s1);

    // s2
    REQUIRE_FALSE(s2 < s2);
    REQUIRE(s2 < s3);
    REQUIRE(s2 < s4);
    REQUIRE(s2 < s5);
    REQUIRE(s2 < s6);
    REQUIRE(s2 < s7);

    // s3
    REQUIRE_FALSE(s3 < s3);
    REQUIRE(s4 < s3);
    REQUIRE(s5 < s3);
    REQUIRE(s6 < s3);
    REQUIRE(s7 < s3);

    // s4
    REQUIRE_FALSE(s4 < s4);
    REQUIRE(s4 < s5);
    REQUIRE(s4 < s6);
    REQUIRE(s4 < s7);

    // s5
    REQUIRE_FALSE(s5 < s5);
    REQUIRE(s5 < s6);
    REQUIRE(s7 < s5);

    // s6
    REQUIRE_FALSE(s6 < s6);
    REQUIRE(s7 < s6);

    // s7
    REQUIRE_FALSE(s7 < s7);
}

