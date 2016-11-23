// @EXPECTED: First type in variant must be default constructible to allow default construction of variant

#include <mapbox/variant.hpp>

// Checks that the first type in a variant must be default constructible to
// make the variant default constructible.

struct no_def_constructor
{

    int value;

    no_def_constructor() = delete;

    no_def_constructor(int v) : value(v) {}
};

int main()
{
    mapbox::util::variant<no_def_constructor> x;
}
