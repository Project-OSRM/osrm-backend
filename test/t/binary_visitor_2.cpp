
#include <mapbox/variant.hpp>

#define NAME_EXT " b-i-d"
using variant_type = mapbox::util::variant<bool, int, double>;

#include "binary_visitor_impl.hpp"
