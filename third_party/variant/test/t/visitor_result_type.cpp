#include <mapbox/variant.hpp>

using namespace mapbox::util;

namespace {

template <typename... T>
struct tag {};

struct deduced_result_visitor
{
    template <typename T>
    tag<T> operator() (T);

    template <typename T>
    tag<T const> operator() (T) const;

    template <typename T, typename U>
    tag<T, U> operator() (T, U);

    template <typename T, typename U>
    tag<T, U const> operator() (T, U) const;
};

struct explicit_result_visitor : deduced_result_visitor
{
    using result_type = tag<float>;
};

// Doing this compile-time test via assignment to typed tag objects gives
// more useful error messages when something goes wrong, than std::is_same
// in a static_assert would. Here if result_of_unary_visit returns anything
// other than the expected type on the left hand side, the conversion error
// message will tell you exactly what it was.

#ifdef __clang__
# pragma clang diagnostic ignored "-Wunused-variable"
#endif

tag<int> d1m = detail::result_of_unary_visit<deduced_result_visitor, int>{};
tag<int const> d1c = detail::result_of_unary_visit<deduced_result_visitor const, int>{};

tag<float> e1m = detail::result_of_unary_visit<explicit_result_visitor, int>{};
tag<float> e1c = detail::result_of_unary_visit<explicit_result_visitor const, int>{};

tag<int, int> d2m = detail::result_of_binary_visit<deduced_result_visitor, int>{};
tag<int, int const> d2c = detail::result_of_binary_visit<deduced_result_visitor const, int>{};

tag<float> e2m = detail::result_of_binary_visit<explicit_result_visitor, int>{};
tag<float> e2c = detail::result_of_binary_visit<explicit_result_visitor const, int>{};

} // namespace
