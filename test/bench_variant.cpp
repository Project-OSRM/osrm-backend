#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "auto_cpu_timer.hpp"

#include <boost/variant.hpp>
#include <mapbox/variant.hpp>

#define TEXT_SHORT "Test"
#define TEXT_LONG "Testing various variant implementations with a longish string ........................................."
#define NUM_SAMPLES 3
//#define BOOST_VARIANT_MINIMIZE_SIZE

using namespace mapbox;

namespace test {

template <typename V>
struct Holder
{
    typedef V value_type;
    std::vector<value_type> data;

    template <typename T>
    void append_move(T&& obj)
    {
        data.emplace_back(std::forward<T>(obj));
    }

    template <typename T>
    void append(T const& obj)
    {
        data.push_back(obj);
    }
};

} // namespace test

struct print
{
    template <typename T>
    void operator()(T const& val) const
    {
        std::cerr << val << ":" << typeid(T).name() << std::endl;
    }
};

template <typename V>
struct dummy : boost::static_visitor<>
{
    dummy(V& v)
        : v_(v) {}

    template <typename T>
    void operator()(T&& val) const
    {
        v_ = std::move(val);
    }
    V& v_;
};

template <typename V>
struct dummy2
{
    dummy2(V& v)
        : v_(v) {}

    template <typename T>
    void operator()(T&& val) const
    {
        v_ = std::move(val);
    }
    V& v_;
};

void run_boost_test(std::size_t runs)
{
    test::Holder<boost::variant<int, double, std::string>> h;
    h.data.reserve(runs);
    for (std::size_t i = 0; i < runs; ++i)
    {
        h.append_move(std::string(TEXT_SHORT));
        h.append_move(std::string(TEXT_LONG));
        h.append_move(123);
        h.append_move(3.14159);
    }

    boost::variant<int, double, std::string> v;
    for (auto const& v2 : h.data)
    {
        dummy<boost::variant<int, double, std::string>> d(v);
        boost::apply_visitor(d, v2);
    }
}

void run_variant_test(std::size_t runs)
{
    test::Holder<util::variant<int, double, std::string>> h;
    h.data.reserve(runs);
    for (std::size_t i = 0; i < runs; ++i)
    {
        h.append_move(std::string(TEXT_SHORT));
        h.append_move(std::string(TEXT_LONG));
        h.append_move(123);
        h.append_move(3.14159);
    }

    util::variant<int, double, std::string> v;
    for (auto const& v2 : h.data)
    {
        dummy2<util::variant<int, double, std::string>> d(v);
        util::apply_visitor(d, v2);
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage:" << argv[0] << " <num-runs>" << std::endl;
        return 1;
    }

#ifndef SINGLE_THREADED
    const std::size_t THREADS = 4;
#endif
    const std::size_t NUM_RUNS = static_cast<std::size_t>(std::stol(argv[1]));

#ifdef SINGLE_THREADED

    for (std::size_t j = 0; j < NUM_SAMPLES; ++j)
    {

        {
            std::cerr << "custom variant: ";
            auto_cpu_timer t;
            run_variant_test(NUM_RUNS);
        }
        {
            std::cerr << "boost variant: ";
            auto_cpu_timer t;
            run_boost_test(NUM_RUNS);
        }
    }

#else
    for (std::size_t j = 0; j < NUM_SAMPLES; ++j)
    {
        {
            typedef std::vector<std::unique_ptr<std::thread>> thread_group;
            typedef thread_group::value_type value_type;
            thread_group tg;
            std::cerr << "custom variant: ";
            auto_cpu_timer timer;
            for (std::size_t i = 0; i < THREADS; ++i)
            {
                tg.emplace_back(new std::thread(run_variant_test, NUM_RUNS));
            }
            std::for_each(tg.begin(), tg.end(), [](value_type& t) {if (t->joinable()) t->join(); });
        }

        {
            typedef std::vector<std::unique_ptr<std::thread>> thread_group;
            typedef thread_group::value_type value_type;
            thread_group tg;
            std::cerr << "boost variant: ";
            auto_cpu_timer timer;
            for (std::size_t i = 0; i < THREADS; ++i)
            {
                tg.emplace_back(new std::thread(run_boost_test, NUM_RUNS));
            }
            std::for_each(tg.begin(), tg.end(), [](value_type& t) {if (t->joinable()) t->join(); });
        }
    }
#endif

    return EXIT_SUCCESS;
}
