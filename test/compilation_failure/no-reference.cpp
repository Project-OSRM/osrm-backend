// @EXPECTED: Variant can not hold reference types

#include <mapbox/variant.hpp>

int main()
{
    mapbox::util::variant<double, int&, long> x{mapbox::util::no_init()};
}
