// @EXPECTED:

#include <mapbox/variant.hpp>

int main()
{
    mapbox::util::variant<int> x;
    mapbox::util::variant<double> y;
    x == y;
}
