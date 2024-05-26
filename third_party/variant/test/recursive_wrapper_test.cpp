#include <cstdlib>
#include <iostream>
#include <string>
#include <typeinfo>
#include <utility>

#include "auto_cpu_timer.hpp"

#include <mapbox/variant.hpp>

using namespace mapbox;

namespace test {

struct add;
struct sub;

template <typename OpTag>
struct binary_op;

typedef util::variant<int,
                      util::recursive_wrapper<binary_op<add>>,
                      util::recursive_wrapper<binary_op<sub>>>
    expression;

template <typename Op>
struct binary_op
{
    expression left; // variant instantiated here...
    expression right;

    binary_op(expression&& lhs, expression&& rhs)
        : left(std::move(lhs)), right(std::move(rhs))
    {
    }
};

struct print
{
    template <typename T>
    void operator()(T const& val) const
    {
        std::cerr << val << ":" << typeid(T).name() << std::endl;
    }
};

struct test
{
    template <typename T>
    std::string operator()(T const& obj) const
    {
        return std::string("TYPE_ID=") + typeid(obj).name();
    }
};

struct calculator
{
  public:
    int operator()(int value) const
    {
        return value;
    }

    int operator()(binary_op<add> const& binary) const
    {
        return util::apply_visitor(*this, binary.left) + util::apply_visitor(*this, binary.right);
    }

    int operator()(binary_op<sub> const& binary) const
    {
        return util::apply_visitor(*this, binary.left) - util::apply_visitor(*this, binary.right);
    }
};

struct to_string
{
  public:
    std::string operator()(int value) const
    {
        return std::to_string(value);
    }

    std::string operator()(binary_op<add> const& binary) const
    {
        return util::apply_visitor(*this, binary.left) + std::string("+") + util::apply_visitor(*this, binary.right);
    }

    std::string operator()(binary_op<sub> const& binary) const
    {
        return util::apply_visitor(*this, binary.left) + std::string("-") + util::apply_visitor(*this, binary.right);
    }
};

} // namespace test

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage" << argv[0] << " <num-iter>" << std::endl;
        return EXIT_FAILURE;
    }

    const std::size_t NUM_ITER = static_cast<std::size_t>(std::stol(argv[1]));

    test::expression sum(test::binary_op<test::add>(2, 3));
    test::expression result(test::binary_op<test::sub>(std::move(sum), 4));

    std::cerr << "TYPE OF RESULT-> " << util::apply_visitor(test::test(), result) << std::endl;

    int total = 0;
    {
        auto_cpu_timer t;
        for (std::size_t i = 0; i < NUM_ITER; ++i)
        {
            total += util::apply_visitor(test::calculator(), result);
        }
    }
    std::cerr << "total=" << total << std::endl;

    std::cerr << util::apply_visitor(test::to_string(), result) << "=" << util::apply_visitor(test::calculator(), result) << std::endl;

    return EXIT_SUCCESS;
}
