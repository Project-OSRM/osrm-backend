
#include <mapbox/variant.hpp>

#define NAME_EXT " b-i-i-d-c-u"
using variant_type = mapbox::util::variant<bool, int, int, double, char, short int>;

#include "binary_visitor_impl.hpp"
