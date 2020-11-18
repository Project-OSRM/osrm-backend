#include "catch.hpp"

#include <osmium/osm/location.hpp>

#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>

// fails on MSVC and doesn't really matter
// static_assert(std::is_literal_type<osmium::Location>::value, "osmium::Location not literal type");

TEST_CASE("Location instantiation with default parameters") {
    const osmium::Location loc;
    REQUIRE_FALSE(loc);
    REQUIRE_FALSE(loc.is_defined());
    REQUIRE(loc.is_undefined());
    REQUIRE_THROWS_AS(loc.lon(), const osmium::invalid_location&);
    REQUIRE_THROWS_AS(loc.lat(), const osmium::invalid_location&);
}

TEST_CASE("Location instantiation with double parameters") {
    const osmium::Location loc1{1.2, 4.5};
    REQUIRE(bool(loc1));
    REQUIRE(loc1.is_defined());
    REQUIRE_FALSE(loc1.is_undefined());
    REQUIRE(12000000 == loc1.x());
    REQUIRE(45000000 == loc1.y());
    REQUIRE(1.2 == Approx(loc1.lon()));
    REQUIRE(4.5 == Approx(loc1.lat()));

    const osmium::Location loc2{loc1};
    REQUIRE(4.5 == Approx(loc2.lat()));

    const osmium::Location loc3 = loc1;
    REQUIRE(4.5 == Approx(loc3.lat()));
}

TEST_CASE("Location instantiation with double parameters constructor with universal initializer") {
    const osmium::Location loc{2.2, 3.3};
    REQUIRE(2.2 == Approx(loc.lon()));
    REQUIRE(3.3 == Approx(loc.lat()));
}

TEST_CASE("Location instantiation with double parameters constructor with initializer list") {
    const osmium::Location loc({4.4, 5.5});
    REQUIRE(4.4 == Approx(loc.lon()));
    REQUIRE(5.5 == Approx(loc.lat()));
}

TEST_CASE("Location instantiation with double parameters operator equal") {
    const osmium::Location loc = {5.5, 6.6};
    REQUIRE(5.5 == Approx(loc.lon()));
    REQUIRE(6.6 == Approx(loc.lat()));
}

TEST_CASE("Location equality") {
    const osmium::Location loc1{1.2, 4.5};
    const osmium::Location loc2{1.2, 4.5};
    const osmium::Location loc3{1.5, 1.5};
    REQUIRE(loc1 == loc2);
    REQUIRE(loc1 != loc3);
}

TEST_CASE("Location order") {
    REQUIRE(osmium::Location(-1.2, 10.0) < osmium::Location(1.2, 10.0));
    REQUIRE(osmium::Location(1.2, 10.0) > osmium::Location(-1.2, 10.0));

    REQUIRE(osmium::Location(10.2, 20.0) < osmium::Location(11.2, 20.2));
    REQUIRE(osmium::Location(10.2, 20.2) < osmium::Location(11.2, 20.0));
    REQUIRE(osmium::Location(11.2, 20.2) > osmium::Location(10.2, 20.0));
}

TEST_CASE("Location validity") {
    REQUIRE(osmium::Location(0.0, 0.0).valid());
    REQUIRE(osmium::Location(1.2, 4.5).valid());
    REQUIRE(osmium::Location(-1.2, 4.5).valid());
    REQUIRE(osmium::Location(-180.0, -90.0).valid());
    REQUIRE(osmium::Location(180.0, -90.0).valid());
    REQUIRE(osmium::Location(-180.0, 90.0).valid());
    REQUIRE(osmium::Location(180.0, 90.0).valid());

    REQUIRE_FALSE(osmium::Location(200.0, 4.5).valid());
    REQUIRE_FALSE(osmium::Location(-1.2, -100.0).valid());
    REQUIRE_FALSE(osmium::Location(-180.0, 90.005).valid());
}


TEST_CASE("Location output to iterator comma separator") {
    char buffer[100];
    const osmium::Location loc{-3.2, 47.3};
    *loc.as_string(buffer, ',') = 0;
    REQUIRE(std::string("-3.2,47.3") == buffer);
}

TEST_CASE("Location output to iterator space separator") {
    char buffer[100];
    const osmium::Location loc{0.0, 7.0};
    *loc.as_string(buffer, ' ') = 0;
    REQUIRE(std::string("0 7") == buffer);
}

TEST_CASE("Location output to iterator check precision") {
    char buffer[100];
    const osmium::Location loc{-179.9999999, -90.0};
    *loc.as_string(buffer, ' ') = 0;
    REQUIRE(std::string("-179.9999999 -90") == buffer);
}

TEST_CASE("Location output to iterator undefined location") {
    char buffer[100];
    const osmium::Location loc;
    REQUIRE_THROWS_AS(loc.as_string(buffer, ','), const osmium::invalid_location&);
}

TEST_CASE("Location output to string comman separator") {
    std::string s;
    const osmium::Location loc{-3.2, 47.3};
    loc.as_string(std::back_inserter(s), ',');
    REQUIRE(s == "-3.2,47.3");
}

TEST_CASE("Location output to string space separator") {
    std::string s;
    const osmium::Location loc{0.0, 7.0};
    loc.as_string(std::back_inserter(s), ' ');
    REQUIRE(s == "0 7");
}

TEST_CASE("Location output to string check precision") {
    std::string s;
    const osmium::Location loc{-179.9999999, -90.0};
    loc.as_string(std::back_inserter(s), ' ');
    REQUIRE(s == "-179.9999999 -90");
}

TEST_CASE("Location output to string undefined location") {
    std::string s;
    const osmium::Location loc;
    REQUIRE_THROWS_AS(loc.as_string(std::back_inserter(s), ','), const osmium::invalid_location&);
}

TEST_CASE("Location output defined") {
    const osmium::Location loc{-3.20, 47.30};
    std::stringstream out;
    out << loc;
    REQUIRE(out.str() == "(-3.2,47.3)");
}

TEST_CASE("Location output undefined") {
    const osmium::Location loc;
    std::stringstream out;
    out << loc;
    REQUIRE(out.str() == "(undefined,undefined)");
}

TEST_CASE("Location hash") {
    if (sizeof(size_t) == 8) {
        REQUIRE(std::hash<osmium::Location>{}({0, 0}) == 0);
        REQUIRE(std::hash<osmium::Location>{}({0, 1}) == 1);
        const int64_t a = std::hash<osmium::Location>{}({1, 0});
        REQUIRE(a == 0x100000000);
        const int64_t b = std::hash<osmium::Location>{}({1, 1});
        REQUIRE(b == 0x100000001);
    } else {
        REQUIRE(std::hash<osmium::Location>{}({0, 0}) == 0);
        REQUIRE(std::hash<osmium::Location>{}({0, 1}) == 1);
        REQUIRE(std::hash<osmium::Location>{}({1, 0}) == 1);
        REQUIRE(std::hash<osmium::Location>{}({1, 1}) == 0);
    }
}

void C(const char* s, int32_t v, const char* r = "") {
    std::string strm{"-"};
    strm += s;
    REQUIRE(std::atof(strm.c_str() + 1) == Approx( v / 10000000.0)); // NOLINT(cert-err34-c)
    REQUIRE(std::atof(strm.c_str()    ) == Approx(-v / 10000000.0)); // NOLINT(cert-err34-c)
    const char* x = strm.c_str() + 1;
    const char** data = &x;
    REQUIRE(osmium::detail::string_to_location_coordinate(data) == v);
    REQUIRE(std::string{*data} == r);
    x = strm.c_str();
    data = &x;
    REQUIRE(osmium::detail::string_to_location_coordinate(data) == -v);
    REQUIRE(std::string{*data} == r);
}

void F(const char* s) {
    std::string strm{"-"};
    strm += s;
    const char* x = strm.c_str();
    const char** data = &x;
    REQUIRE_THROWS_AS(osmium::detail::string_to_location_coordinate(data), const osmium::invalid_location&);
    ++x;
    data = &x;
    REQUIRE_THROWS_AS(osmium::detail::string_to_location_coordinate(data), const osmium::invalid_location&);
}

TEST_CASE("Parsing coordinates from strings") {
    F("x");
    F(".");
    F(".e2");
    F("--");
    F("");
    F(" ");
    F(" 123");

    C("123 ", 1230000000, " ");
    C("123x", 1230000000, "x");
    C("1.2x",   12000000, "x");

    C("0",              0);

    C("1",       10000000);
    C("2",       20000000);

    C("9",       90000000);
    C("10",     100000000);
    C("11",     110000000);

    C("90",     900000000);
    C("100",   1000000000);
    C("101",   1010000000);

    C("00",             0);
    C("01",      10000000);
    C("001",     10000000);
    C("0001",    10000000);

    F("1234");
    F("1234.");
    F("12345678901234567890");
    F("1.1234568111111111111111111111111111111");
    F("112.34568111111111111111111111111111111");

    C("0.",             0);
    C(".0",             0);
    C("0.0",            0);
    C("1.",      10000000);
    C("1.0",     10000000);
    C("1.2",     12000000);
    C("0.1",      1000000);
    C(".1",       1000000);
    C("0.01",      100000);
    C(".01",       100000);
    C("0.001",      10000);
    C("0.0001",      1000);
    C("0.00001",      100);
    C("0.000001",      10);
    C("0.0000001",      1);

    C("1.1234567",  11234567);
    C("1.12345670", 11234567);
    C("1.12345674", 11234567);
    C("1.123456751", 11234568);
    C("1.12345679", 11234568);
    C("1.12345680", 11234568);
    C("1.12345681", 11234568);

    C("180.0000000",  1800000000);
    C("180.0000001",  1800000001);
    C("179.9999999",  1799999999);
    C("179.99999999", 1800000000);
    C("200.123",      2001230000);
    C("214.7483647",  2147483647);

    C("8.109E-4" , 8109);
    C("8.1090E-4" , 8109);
    C("8.10909E-4" , 8109);
    C("8.109095E-4" , 8109);
    C("8.1090959E-4" , 8109);
    C("8.10909598E-4" , 8109);
    C("8.109095988E-4" , 8109);
    C("8.1090959887E-4" , 8109);
    C("8.10909598870E-4" , 8109);
    C("8.109095988709E-4" , 8109);
    C("8.1090959887098E-4" , 8109);
    C("8.10909598870983E-4" , 8109);
    C("8.109095988709837E-4" , 8109);
    C("81.09095988709837E-4" , 81091);
    C("810.9095988709837E-4" , 810910);
    C(".8109095988709837E-4" , 811);
    C(".08109095988709837E-4" , 81);

    C("1e2",   1000000000);
    C("1e1",    100000000);
    C("1e0",     10000000);
    C("1e-1",     1000000);
    C("1e-2",      100000);
    C("1e-3",       10000);
    C("1e-4",        1000);
    C("1e-5",         100);
    C("1e-6",          10);
    C("1e-7",           1);

    C("1.0e2",   1000000000);
    C("1.e2",    1000000000);
    C("1.1e1",    110000000);
    C("0.1e1",     10000000);
    C(".1e1",      10000000);
    C("1.2e0",     12000000);
    C("1.9e-1",     1900000);
    C("2.0e-2",      200000);
    C("2.1e-3",       21000);
    C("9.0e-4",        9000);
    C("9.1e-5",         910);
    C("1.0e-6",          10);
    C("1.0e-7",           1);
    C("1.4e-7",           1);
    C("1.5e-7",           2);
    C("1.9e-7",           2);
    C("0.5e-7",           1);
    C("0.1e-7",           0);
    C("0.0e-7",           0);
    C("1.9e-8",           0);
    C("1.9e-9",           0);
    C("1.9e-10",          0);

    F("e");
    F(" e");
    F(" 1.1e2");
    F("1.0e3");
    F("5e4");
    F("5.0e2");
    F("3e2");
    F("1e");
    F("1e-");
    F("1e1234567");
    F("0.5e");
    F("1e10");

    C("1e2 ",   1000000000, " ");
    C("1.1e2 ", 1100000000, " ");
    C("1.1e2x", 1100000000, "x");
    C("1.1e2:", 1100000000, ":");
}

TEST_CASE("Parsing min coordinate from string") {
    const char* minval = "-214.7483648";
    const char** data = &minval;
    REQUIRE(osmium::detail::string_to_location_coordinate(data) == std::numeric_limits<int32_t>::min());
}

TEST_CASE("Writing zero coordinate into string") {
    std::string buffer;
    osmium::detail::append_location_coordinate_to_string(std::back_inserter(buffer), 0);
    REQUIRE(buffer == "0");
}

void CW(int32_t v, const char* s) {
    std::string buffer;

    osmium::detail::append_location_coordinate_to_string(std::back_inserter(buffer), v);
    REQUIRE(buffer == s);
    buffer.clear();
    osmium::detail::append_location_coordinate_to_string(std::back_inserter(buffer), -v);
    REQUIRE(buffer[0] == '-');
    REQUIRE_FALSE(std::strcmp(buffer.c_str() + 1, s));
}

TEST_CASE("Writing coordinate into string") {
    CW(  10000000, "1");
    CW(  90000000, "9");
    CW( 100000000, "10");
    CW(1000000000, "100");
    CW(2000000000, "200");

    CW(   1000000, "0.1");
    CW(    100000, "0.01");
    CW(     10000, "0.001");
    CW(      1000, "0.0001");
    CW(       100, "0.00001");
    CW(        10, "0.000001");
    CW(         1, "0.0000001");

    CW(   1230000, "0.123");
    CW(   9999999, "0.9999999");
    CW(  40101010, "4.010101");
    CW( 494561234, "49.4561234");
    CW(1799999999, "179.9999999");

    CW(2147483647, "214.7483647");
}

TEST_CASE("Writing min coordinate into string") {
    std::string buffer;

    osmium::detail::append_location_coordinate_to_string(std::back_inserter(buffer), std::numeric_limits<int32_t>::min());
    REQUIRE(buffer == "-214.7483648");
}

TEST_CASE("set lon/lat from string") {
    osmium::Location loc;
    REQUIRE(loc.is_undefined());
    REQUIRE_FALSE(loc.is_defined());
    REQUIRE_FALSE(loc.valid());

    loc.set_lon("1.2");
    REQUIRE_FALSE(loc.is_undefined());
    REQUIRE(loc.is_defined());
    REQUIRE_FALSE(loc.valid());

    loc.set_lat("3.4");
    REQUIRE_FALSE(loc.is_undefined());
    REQUIRE(loc.is_defined());
    REQUIRE(loc.valid());

    REQUIRE(loc.lon() == Approx(1.2));
    REQUIRE(loc.lat() == Approx(3.4));
}

TEST_CASE("set lon/lat from string with trailing characters") {
    osmium::Location loc;
    REQUIRE_THROWS_AS(loc.set_lon("1.2x"), const osmium::invalid_location&);
    REQUIRE_THROWS_AS(loc.set_lat("3.4e1 "), const osmium::invalid_location&);
}

TEST_CASE("set lon/lat from string with trailing characters using partial") {
    osmium::Location loc;
    const char* x = "1.2x";
    const char* y = "3.4 ";
    loc.set_lon_partial(&x);
    loc.set_lat_partial(&y);
    REQUIRE(loc.lon() == Approx(1.2));
    REQUIRE(loc.lat() == Approx(3.4));
    REQUIRE(*x == 'x');
    REQUIRE(*y == ' ');
}

