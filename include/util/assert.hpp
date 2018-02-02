#ifndef OSRM_UTIL_ASSERT_HPP
#define OSRM_UTIL_ASSERT_HPP

#include <iostream>
#include <string>

#include "util/to_osm_link.hpp"

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
            std::cerr << "[Location] " << ::osrm::util::toOSMLink(loc) << '\n';                    \
        }                                                                                          \
        BOOST_ASSERT_MSG(cond, msg);                                                               \
    } while (0)

#define OSRM_ASSERT(cond, loc) OSRM_ASSERT_MSG(cond, loc, "")

#else

#define OSRM_ASSERT_MSG(cond, coordinate, msg)                                                     \
    do                                                                                             \
    {                                                                                              \
        (void)(cond);                                                                              \
        (void)(coordinate);                                                                        \
        (void)(msg);                                                                               \
    } while (0)
#define OSRM_ASSERT(cond, coordinate) OSRM_ASSERT_MSG(cond, coordinate, "")

#endif

#endif
