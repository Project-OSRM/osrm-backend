#include <boost/test/unit_test.hpp>

#include <cmath>
#include <vector>
#include <iostream>

BOOST_AUTO_TEST_CASE(rounded_consistency_between_double_and_long_double)
{
    // Representative sample durations (seconds) observed and chosen to exercise borderline cases
    const std::vector<double> samples = {
        1.5026,
        3.60139,
        2.49601,
        0.9999,
        1.25,
        1.35,
        1.45,
        1.55,
        1.65,
        12345.678901234,
        0.1,
        0.2,
        0.3,
        0.4,
        0.5,
        0.6,
        0.7,
        0.8,
        0.9,
    };

    for (const auto d : samples)
    {
        const long double ld = static_cast<long double>(d);
        // method A: std::round on double
        const long a = static_cast<long>(std::round(d * 10.0));
        // method B: long double floor-based rounding
        const long b = static_cast<long>(std::floor(ld * 10.0L + 0.5L));

        if (a != b)
        {
            std::cerr << "Mismatch for d=" << d << ": std::round->" << a
                      << ", floor(long double)->" << b << std::endl;
        }

        BOOST_CHECK_MESSAGE(a == b, "Rounding mismatch for value: " << d << " (a=" << a
                                                        << ", b=" << b << ")");
    }
}
