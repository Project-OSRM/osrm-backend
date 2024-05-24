#include <typeinfo>
#include <utility>

#include <mapbox/variant.hpp>

using namespace mapbox;

namespace test {

struct t_noexcept_true_1 {
  t_noexcept_true_1(t_noexcept_true_1&&) noexcept = default;
  t_noexcept_true_1& operator=(t_noexcept_true_1&&) noexcept = default;
};

struct t_noexcept_true_2 {
  t_noexcept_true_2(t_noexcept_true_2&&) noexcept = default;
  t_noexcept_true_2& operator=(t_noexcept_true_2&&) noexcept = default;
};

struct t_noexcept_false_1 {
  t_noexcept_false_1(t_noexcept_false_1&&) noexcept(false) {}
  t_noexcept_false_1& operator=(t_noexcept_false_1&&) noexcept(false) { return *this; }
};

using should_be_no_throw_copyable = util::variant<t_noexcept_true_1, t_noexcept_true_2>;
static_assert(std::is_nothrow_move_assignable<should_be_no_throw_copyable>::value,
              "variants with no-throw move assignable types should be "
              "no-throw move nothrow assignable");

using should_be_no_throw_assignable = util::variant<t_noexcept_true_1, t_noexcept_true_2>;
static_assert(std::is_nothrow_move_constructible<should_be_no_throw_assignable>::value,
              "variants with no-throw move assignable types should be "
              "no-throw move nothrow assignable");

using should_not_be_no_throw_copyable = util::variant<t_noexcept_true_1, t_noexcept_false_1>;
static_assert(not std::is_nothrow_move_assignable<should_not_be_no_throw_copyable>::value,
              "variants with no-throw move assignable types should be "
              "no-throw move nothrow assignable");

using should_not_be_no_throw_assignable = util::variant<t_noexcept_true_1, t_noexcept_false_1>;
static_assert(not std::is_nothrow_move_constructible<should_not_be_no_throw_assignable>::value,
              "variants with no-throw move assignable types should be "
              "no-throw move nothrow assignable");


// this type cannot be nothrow converted from either of its types, even the nothrow moveable one,
// because the conversion operator moves the whole variant.
using convertable_test_type = util::variant<t_noexcept_true_1, t_noexcept_false_1>;

// this type can be nothrow converted from either of its types.
using convertable_test_type_2 = util::variant<t_noexcept_true_1, t_noexcept_true_2>;

static_assert(not std::is_nothrow_assignable<convertable_test_type, t_noexcept_true_1>::value,
              "variants with noexcept(true) move constructible types should be nothrow-convertible "
              "from those types only IF the variant itself is nothrow_move_assignable");

static_assert(not std::is_nothrow_assignable<convertable_test_type, t_noexcept_false_1>::value,
              "variants with noexcept(false) move constructible types should not be nothrow-convertible "
              "from those types");

static_assert(std::is_nothrow_assignable<convertable_test_type_2, t_noexcept_true_2>::value,
              "variants with noexcept(true) move constructible types should be nothrow-convertible "
              "from those types only IF the variant itself is nothrow_move_assignable");


} // namespace test
