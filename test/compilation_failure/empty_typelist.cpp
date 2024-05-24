// @EXPECTED: Template parameter type list of variant can not be empty

#include <mapbox/variant.hpp>

// Empty type list should not work.

int main()
{
    mapbox::util::variant<> x;
}
