#include "util/vector_view.hpp"
#include "util/typedefs.hpp"

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <numeric>
#include <random>

BOOST_AUTO_TEST_SUITE(vector_view_test)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(rw_short)
{
    std::size_t num_elements = 1000;
    std::unique_ptr<char[]> data = std::make_unique<char[]>(sizeof(std::uint16_t) * num_elements);
    util::vector_view<std::uint16_t> view(reinterpret_cast<std::uint16_t *>(data.get()),
                                          num_elements);
    std::vector<std::uint16_t> reference;

    std::mt19937 rng;
    rng.seed(1337);
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, (1UL << 16));

    for (std::size_t i = 0; i < num_elements; i++)
    {
        auto r = dist(rng);
        view[i] = r;
        reference.push_back(r);
    }

    for (std::size_t i = 0; i < num_elements; i++)
    {
        BOOST_CHECK_EQUAL(view[i], reference[i]);
    }
}

BOOST_AUTO_TEST_CASE(rw_bool)
{
    std::size_t num_elements = 1000;
    auto data = std::make_unique<typename vector_view<bool>::Word[]>(
        (num_elements + sizeof(typename vector_view<bool>::Word) - 1) /
        sizeof(typename vector_view<bool>::Word));
    util::vector_view<bool> view(reinterpret_cast<typename vector_view<bool>::Word *>(data.get()),
                                 num_elements);
    std::vector<bool> reference;

    std::mt19937 rng;
    rng.seed(1337);
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, 2);

    for (std::size_t i = 0; i < num_elements; i++)
    {
        auto r = dist(rng);
        view[i] = r;
        reference.push_back(r);
    }

    for (std::size_t i = 0; i < num_elements; i++)
    {
        BOOST_CHECK_EQUAL(view[i], reference[i]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
