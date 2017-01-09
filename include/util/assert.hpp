#ifndef OSRM_UTIL_ASSERT_HPP
#define OSRM_UTIL_ASSERT_HPP

#include <iostream>
#include <string>

#include "util/coordinate.hpp"

#include <boost/assert.hpp>

// Enhances BOOST_ASSERT / BOOST_ASSERT_MSG with convenience location printing
// - OSRM_ASSERT(cond, coordinate)
// - OSRM_ASSERT_MSG(cond, coordinate, msg)

#ifdef BOOST_ENABLE_ASSERT_HANDLER

#define OSRM_ASSERT_MSG(cond, loc, msg)                                                            \
    do                                                                                             \
    {                                                                                              \
        if (!static_cast<bool>(cond))                                                              \
        {                                                                                          \
            ::osrm::util::FloatCoordinate c_(loc);                                                 \
            std::cerr << "[Location] "                                                             \
                      << "http://www.openstreetmap.org/?mlat=" << c_.lat << "&mlon=" << c_.lon     \
                      << "#map=19/" << c_.lat << "/" << c_.lon << '\n';                            \
        }                                                                                          \
        BOOST_ASSERT_MSG(cond, msg);                                                               \
    } while (0)

#define OSRM_ASSERT(cond, loc) OSRM_ASSERT_MSG(cond, loc, "")

#else

#define OSRM_ASSERT_MSG(cond, coordinate, msg)                                                     \
    do                                                                                             \
    {                                                                                              \
        (void)cond;                                                                                \
        (void)coordinate;                                                                          \
        (void)msg;                                                                                 \
    } while (0)
#define OSRM_ASSERT(cond, coordinate) OSRM_ASSERT_MSG(cond, coordinate, "")

#endif

#endif
