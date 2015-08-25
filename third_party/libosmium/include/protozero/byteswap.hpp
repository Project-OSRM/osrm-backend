#ifndef PROTOZERO_BYTESWAP_HPP
#define PROTOZERO_BYTESWAP_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

#include <cassert>

namespace protozero {

template <int N>
inline void byteswap(const char* /*data*/, char* /*result*/) {
    assert(false);
}

template <>
inline void byteswap<1>(const char* data, char* result) {
    result[0] = data[0];
}

template <>
inline void byteswap<4>(const char* data, char* result) {
    result[3] = data[0];
    result[2] = data[1];
    result[1] = data[2];
    result[0] = data[3];
}

template <>
inline void byteswap<8>(const char* data, char* result) {
    result[7] = data[0];
    result[6] = data[1];
    result[5] = data[2];
    result[4] = data[3];
    result[3] = data[4];
    result[2] = data[5];
    result[1] = data[6];
    result[0] = data[7];
}

} // end namespace protozero

#endif // PROTOZERO_BYTESWAP_HPP
