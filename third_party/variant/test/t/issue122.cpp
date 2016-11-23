#include "catch.hpp"

#include <mapbox/variant.hpp>
#include <mapbox/variant_io.hpp>

// https://github.com/mapbox/variant/issues/122

struct X
{
    template <typename ValueType>
    X(const ValueType&)  {}
};


TEST_CASE("Correctly choose appropriate constructor", "[variant]")
{
    mapbox::util::variant<X, int> a{123};
    decltype(a) b(a);
    REQUIRE(a.which() == b.which());
}
