
#include <mapbox/variant.hpp>

#define NAME_EXT " i-d-b"
using variant_type = mapbox::util::variant<int, double, bool>;

#include "binary_visitor_impl.hpp"
