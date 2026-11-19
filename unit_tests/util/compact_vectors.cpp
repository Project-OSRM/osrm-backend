#include "util/packed_vector.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <vector>

BOOST_AUTO_TEST_SUITE(compact_vectors)

BOOST_AUTO_TEST_CASE(adaptive_widths_sentinels_and_reverse_iteration)
{
    using Iterator = osrm::util::PackedVector<std::uint32_t, 22>::const_iterator;
    static_assert(sizeof(Iterator) == 16);
    for (unsigned width = 0; width <= 22; ++width)
        for (bool sentinel : {false, true})
        {
            if (!width && sentinel)
                continue;
            const std::uint64_t mask = (1ULL << width) - 1;
            for (unsigned base : {0u, 1u, 31u, 63u, 64u, 127u})
            {
                std::vector<std::uint64_t> words((base + 300 * width + 63) / 64 + 1, 0);
                std::vector<std::uint32_t> expected;
                for (unsigned i = 0; i < 300; ++i)
                {
                    const auto raw = (i * 7919ULL) & mask;
                    expected.push_back(sentinel && raw == mask ? 4194303 : raw);
                    const auto bit = base + i * width;
                    words[bit / 64] |= raw << (bit % 64);
                    if (bit % 64 + width > 64)
                        words[bit / 64 + 1] |= raw >> (64 - bit % 64);
                }
                const auto begin = Iterator::encoded(words.data(), base, width, sentinel, 0);
                const auto end = Iterator::encoded(words.data(), base, width, sentinel, 300);
                BOOST_CHECK_EQUAL(end - begin, 300);
                BOOST_CHECK_EQUAL(begin - end, -300);
                BOOST_CHECK(end - 300 == begin);
                auto reverse = std::make_reverse_iterator(end);
                for (unsigned i = 0; i < 300; ++i, ++reverse)
                {
                    BOOST_CHECK_EQUAL(begin[i], expected[i]);
                    BOOST_CHECK_EQUAL(*(end - (300 - i)), expected[i]);
                    BOOST_CHECK_EQUAL(*reverse, expected[299 - i]);
                }
            }
        }
}

BOOST_AUTO_TEST_SUITE_END()
