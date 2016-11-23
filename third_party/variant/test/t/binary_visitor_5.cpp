
#include <mapbox/variant.hpp>

#define NAME_EXT " b-i-c-d-i"
using variant_type = mapbox::util::variant<bool, int, char, double, int>;

#include "binary_visitor_impl.hpp"
