// @EXPECTED:

#include <mapbox/variant.hpp>

int main()
{
    mapbox::util::variant<int, double> x;
    x.is<std::string>();
}
