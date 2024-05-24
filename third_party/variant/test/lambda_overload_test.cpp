#include <iostream>
#include <vector>

#include <mapbox/variant.hpp>
#include <mapbox/variant_visitor.hpp>

#if __cplusplus >= 201402L
#define HAS_CPP14_SUPPORT
#endif

using namespace mapbox::util;

template <typename T>
struct tag
{
    static void dump(const char* prefix)
    {
        std::cout << prefix << ": " << typeid(tag<T>).name() << std::endl;
    }
};

template <typename Left, typename Right>
using Either = mapbox::util::variant<Left, Right>;

struct Response
{
};

struct Error
{
};

void test_lambda_overloads()
{
    Either<Error, Response> rv;

    rv = Response{};

    auto visitor = make_visitor([](Response) { std::cout << "Response\n"; }, //
                                [](Error) { std::cout << "Error\n"; });      //
    apply_visitor(visitor, rv);
}

void test_lambda_overloads_capture()
{
    Either<Error, Response> rv;

    rv = Error{};

    int ok = 0;
    int err = 0;

    auto visitor = make_visitor([&](Response) { ok += 1; }, //
                                [&](Error) { err += 1; });  //
    apply_visitor(visitor, rv);

    std::cout << "Got " << ok << " ok, " << err << " err" << std::endl;
}

void test_singleton_variant()
{

    variant<int> singleton;
    apply_visitor(make_visitor([](int) {}), singleton);
}

// See #180
struct test_call_nonconst_member_visitor
{
    template <typename T>
    void operator() (T & obj) const
    {
        tag<decltype(obj)>::dump("test_call_nonconst_member: visitor");
        obj.foo();
    }
};

void test_call_nonconst_member()
{
    struct object
    {
        void foo() { val = 42;}
        int val = 0;
    };

    variant<object> v = object{};
    apply_visitor(test_call_nonconst_member_visitor{}, v);

#ifdef HAS_CPP14_SUPPORT
    apply_visitor([](auto& obj)
                  {
                      tag<decltype(obj)>::dump("test_call_nonconst_member: lambda");
                      obj.foo();
                  }, v);
#endif
}

void test_lambda_overloads_sfinae()
#ifdef HAS_CPP14_SUPPORT
{
    variant<int, float, std::vector<int>> var;

    auto visitor = make_visitor([](auto range) -> decltype(std::begin(range), void()) {
                                    for (auto each : range)
                                        std::cout << each << ' '; },
                                [](auto x) -> decltype(std::cout << x, void()) {
                                    std::cout << x << std::endl;
                                });

    var = 1;
    apply_visitor(visitor, var);

    var = 2.f;
    apply_visitor(visitor, var);

    var = std::vector<int>{4, 5, 6};
    apply_visitor(visitor, var);
}
#else
{
}
#endif

void test_match_singleton()
{
    variant<int> singleton = 5;
    singleton.match([](int) {});
    
    auto lambda = [](int) {};
    singleton.match(lambda);
}

void test_match_overloads()
{
    Either<Error, Response> rv;

    rv = Response{};

    rv.match([](Response) { std::cout << "Response\n"; }, //
             [](Error) { std::cout << "Error\n"; });      //
}

void test_match_overloads_capture()
{
    Either<Error, Response> rv;

    rv = Error{};

    int ok = 0;
    int err = 0;

    rv.match([&](Response) { ok += 1; }, //
             [&](Error) { err += 1; });  //

    std::cout << "Got " << ok << " ok, " << err << " err" << std::endl;
}

struct MovableOnly
{
    MovableOnly() = default;
    
    MovableOnly(MovableOnly&&) = default;
    MovableOnly& operator=(MovableOnly&&) = default;
};

struct MovableCopyable
{
    MovableCopyable() = default;
    
    MovableCopyable(MovableCopyable&&) = default;
    MovableCopyable& operator=(MovableCopyable&&) = default;
    
    MovableCopyable(const MovableCopyable&) = default;
    MovableCopyable& operator=(const MovableCopyable&) = default;
};

void test_match_overloads_init_capture()
#ifdef HAS_CPP14_SUPPORT
{
    Either<Error, Response> rv;

    rv = Error{};

    rv.match([p = MovableOnly{}](auto&&) {});
    {
        auto lambda = [p = MovableCopyable{}](auto&&) {};
        rv.match(lambda);
    
        rv.match([p = MovableOnly{}](Response) { std::cout << "Response\n"; },
                 [p = MovableOnly{}](Error) { std::cout << "Error\n"; });
    }    
    {
        auto lambda = [](Error) { std::cout << "Error\n"; };
        rv.match([p = MovableOnly{}](Response) { std::cout << "Response\n"; },
                 lambda);
        rv.match(lambda,
                 [p = MovableOnly{}](Response) { std::cout << "Response\n"; });
    }
}
#else
{
}
#endif

// See #140
void test_match_overloads_otherwise()
#ifdef HAS_CPP14_SUPPORT
{

    struct Center
    {
    };
    struct Indent
    {
    };
    struct Justify
    {
    };
    struct None
    {
    };

    using Properties = mapbox::util::variant<Center, Indent, Justify, None>;

    Properties props = Justify{};

    props.match([&](Center) { std::cout << "Center\n"; },     //
                [&](Indent) { std::cout << "Indent\n"; },     //
                [&](auto&&) { std::cout << "Otherwise\n"; }); //
}
#else
{
}
#endif

template <typename>
struct Moveable
{
    Moveable() = default; // Default constructible

    Moveable(const Moveable&) = delete;            // Disable copy ctor
    Moveable& operator=(const Moveable&) = delete; // Disable copy assign op

    Moveable(Moveable&&) = default;            // Enable move ctor
    Moveable& operator=(Moveable&&) = default; // Enable move assign op
};

void test_match_move_out_of_variant()
{
    // Distinguishable at type level
    using T1 = Moveable<struct Tag1>;
    using T2 = Moveable<struct Tag2>;
    using T3 = mapbox::util::recursive_wrapper<int>;

    mapbox::util::variant<T1, T2> v = T1{};

    std::move(v).match([](T1&&) {},  // Consume T1 by value
                       [](T2&&) {}); // Consume T2 by value

    mapbox::util::variant<T3, T2> w = T2{};

    std::move(w).match([](int&&) {}, // Consume unwrapped int
                       [](T2&&) {}); // Consume T2 by value
}

int main()
{
    test_lambda_overloads();
    test_singleton_variant();
    test_call_nonconst_member();
    test_lambda_overloads_capture();
    test_lambda_overloads_sfinae();

    test_match_singleton();
    test_match_overloads();
    test_match_overloads_capture();
    test_match_overloads_init_capture();
    test_match_overloads_otherwise();
    test_match_move_out_of_variant();
}

#undef HAS_CPP14_SUPPORT
