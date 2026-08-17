#include "util/function_ref.hpp"

#include <boost/test/unit_test.hpp>

#include <string>
#include <type_traits>

BOOST_AUTO_TEST_SUITE(function_ref_test)

using namespace osrm;
using namespace osrm::util;

namespace
{
struct EdgeData
{
    bool forward;
    bool backward;
};

int applyTwice(FunctionRef<int(int)> fn, int value) { return fn(fn(value)); }

bool freeFunction(const EdgeData &data) { return data.forward; }
} // namespace

BOOST_AUTO_TEST_CASE(calls_a_captureless_lambda)
{
    const EdgeData data{true, false};

    BOOST_CHECK_EQUAL(FunctionRef<bool(const EdgeData &)>{[](const EdgeData &edge)
                                                          { return edge.forward; }}(data),
                      true);
    BOOST_CHECK_EQUAL(FunctionRef<bool(const EdgeData &)>{[](const EdgeData &edge)
                                                          { return edge.backward; }}(data),
                      false);
}

BOOST_AUTO_TEST_CASE(reads_the_captured_state_through_the_reference)
{
    // The callable is referred to, not copied, so state captured by reference is
    // observed at call time rather than at construction time.
    int captured = 1;
    const auto lambda = [&captured](int value) { return value * captured; };
    const FunctionRef<int(int)> fn = lambda;

    BOOST_CHECK_EQUAL(fn(10), 10);
    captured = 3;
    BOOST_CHECK_EQUAL(fn(10), 30);
}

BOOST_AUTO_TEST_CASE(mutates_the_referenced_callable)
{
    int calls = 0;
    auto counter = [&calls](int value)
    {
        ++calls;
        return value;
    };
    const FunctionRef<int(int)> fn = counter;

    fn(0);
    fn(0);
    BOOST_CHECK_EQUAL(calls, 2);
}

BOOST_AUTO_TEST_CASE(passes_a_temporary_lambda_as_an_argument)
{
    // The argument outlives the call, which is the case FunctionRef is for.
    BOOST_CHECK_EQUAL(applyTwice([](int value) { return value + 3; }, 1), 7);
}

BOOST_AUTO_TEST_CASE(forwards_arguments_without_copying)
{
    static int copies = 0;
    struct Counted
    {
        Counted() = default;
        Counted(const Counted &) { ++copies; }
    };

    const Counted counted;
    const FunctionRef<bool(const Counted &)> fn = [](const Counted &) { return true; };

    BOOST_CHECK_EQUAL(fn(counted), true);
    BOOST_CHECK_EQUAL(copies, 0);
}

BOOST_AUTO_TEST_CASE(returns_void_and_non_trivial_types)
{
    bool ran = false;
    const auto sink = [&ran](int) { ran = true; };
    FunctionRef<void(int)>{sink}(0);
    BOOST_CHECK(ran);

    const auto name = [](int value) { return std::to_string(value); };
    BOOST_CHECK_EQUAL(FunctionRef<std::string(int)>{name}(42), "42");
}

BOOST_AUTO_TEST_CASE(is_two_pointers_and_trivially_copyable)
{
    BOOST_CHECK_EQUAL(sizeof(FunctionRef<int(int)>), 2 * sizeof(void *));
    BOOST_CHECK(std::is_trivially_copyable_v<FunctionRef<int(int)>>);
    BOOST_CHECK((
        std::is_nothrow_constructible_v<FunctionRef<int(int)>, decltype([](int i) { return i; })>));
}

BOOST_AUTO_TEST_CASE(rejects_what_it_cannot_point_at)
{
    // A plain function has no object to take the address of. Rejecting it in the
    // constraint keeps the diagnostic readable instead of failing inside the body.
    BOOST_CHECK(
        (!std::is_constructible_v<FunctionRef<bool(const EdgeData &)>, decltype(freeFunction) &>));

    // A function pointer is an object, so it is fine.
    BOOST_CHECK((
        std::is_constructible_v<FunctionRef<bool(const EdgeData &)>, bool (*&)(const EdgeData &)>));

    // Signature mismatches are rejected rather than silently converted.
    BOOST_CHECK((!std::is_constructible_v<FunctionRef<int(int)>,
                                          decltype([](const std::string &) { return 0; }) &>));
}

BOOST_AUTO_TEST_CASE(calls_through_a_function_pointer_object)
{
    const EdgeData data{true, false};
    bool (*pointer)(const EdgeData &) = &freeFunction;
    const FunctionRef<bool(const EdgeData &)> fn = pointer;

    BOOST_CHECK_EQUAL(fn(data), true);
}

BOOST_AUTO_TEST_SUITE_END()
