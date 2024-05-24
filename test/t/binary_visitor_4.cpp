
#include <mapbox/variant.hpp>

#define NAME_EXT " b-i-d-c"
using variant_type = mapbox::util::variant<bool, int, double, char>;

#include "binary_visitor_impl.hpp"
