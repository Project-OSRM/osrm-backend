#ifndef OSMIUM_TEST_CRC_HPP
#define OSMIUM_TEST_CRC_HPP

#ifdef OSMIUM_TEST_CRC_USE_BOOST

/* Use the CRC32 implementation from boost. */
#include <boost/crc.hpp>
using crc_type = boost::crc_32_type;

#else

/* Use the CRC32 implementation from zlib. */
#include <osmium/osm/crc_zlib.hpp>
using crc_type = osmium::CRC_zlib;

#endif

#endif // OSMIUM_TEST_CRC_HPP
