
// @EXPECTED: enable_if

#include <variant.hpp>

int main()
{
    mapbox::util::variant<int, double> x;
    x.get<std::string>();
}
