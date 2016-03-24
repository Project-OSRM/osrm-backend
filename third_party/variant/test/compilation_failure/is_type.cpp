
// @EXPECTED: invalid type in T in `is<T>()` for this variant

#include <variant.hpp>

int main()
{
    mapbox::util::variant<int, double> x;
    x.is<std::string>();
}
