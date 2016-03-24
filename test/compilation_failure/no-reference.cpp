
// @EXPECTED: Variant can not hold reference types

#include <variant.hpp>

int main()
{
    mapbox::util::variant<double, int&, long> x{mapbox::util::no_init()};
}
