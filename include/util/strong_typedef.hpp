/*

Copyright (c) 2016, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef STRONG_TYPEDEF_HPP
#define STRONG_TYPEDEF_HPP

#include <functional>
#include <iostream>
#include <limits>
#include <type_traits>

namespace osrm
{

/* Creates strongly typed wrappers around scalar types.
 * Useful for stopping accidental assignment of lats to lons,
 * etc.  Also clarifies what this random "int" value is
 * being used for.
 */
#define OSRM_STRONG_TYPEDEF(From, To)                                                              \
    struct To final                                                                                \
    {                                                                                              \
        using value_type = From;                                                                   \
        static_assert(std::is_arithmetic<From>(), "");                                             \
        From __value;                                                                              \
        friend std::ostream &operator<<(std::ostream &stream, const To &inst);                     \
                                                                                                   \
        explicit operator From &() { return __value; }                                             \
        explicit operator From() const { return __value; }                                         \
        To operator+(const To rhs) const { return To{static_cast<From>(__value + rhs.__value)}; }  \
        To operator-(const To rhs) const { return To{static_cast<From>(__value - rhs.__value)}; }  \
        To operator*(const To rhs) const { return To{static_cast<From>(__value * rhs.__value)}; }  \
        To operator*(const double rhs) const { return To{static_cast<From>(__value * rhs)}; }      \
        To operator/(const double rhs) const { return To{static_cast<From>(__value / rhs)}; }      \
        bool operator<(const To rhs) const { return __value < rhs.__value; }                       \
        bool operator>(const To rhs) const { return __value > rhs.__value; }                       \
        bool operator<=(const To rhs) const { return __value <= rhs.__value; }                     \
        bool operator>=(const To rhs) const { return __value >= rhs.__value; }                     \
        bool operator==(const To rhs) const { return __value == rhs.__value; }                     \
        bool operator!=(const To rhs) const { return __value != rhs.__value; }                     \
        To &operator+=(const To rhs)                                                               \
        {                                                                                          \
            __value += rhs.__value;                                                                \
            return *this;                                                                          \
        }                                                                                          \
        To &operator-=(const To rhs)                                                               \
        {                                                                                          \
            __value -= rhs.__value;                                                                \
            return *this;                                                                          \
        }                                                                                          \
        template <typename T = From>                                                               \
        typename std::enable_if<std::is_signed<T>::value, To>::type operator-() const              \
        {                                                                                          \
            return To{-__value};                                                                   \
        }                                                                                          \
    };                                                                                             \
    static_assert(std::is_trivial<To>(), #To " is not a trivial type");                            \
    static_assert(std::is_standard_layout<To>(), #To " is not a standart layout");                 \
    static_assert(std::is_pod<To>(), #To " is not a POD layout");                                  \
    inline std::ostream &operator<<(std::ostream &stream, const To &inst)                          \
    {                                                                                              \
        return stream << inst.__value;                                                             \
    }

#define OSRM_STRONG_TYPEDEF_HASHABLE(From, To)                                                     \
    namespace std                                                                                  \
    {                                                                                              \
    template <> struct hash<To>                                                                    \
    {                                                                                              \
        std::size_t operator()(const To &k) const                                                  \
        {                                                                                          \
            return std::hash<From>()(static_cast<const From>(k));                                  \
        }                                                                                          \
    };                                                                                             \
    }

#define OSRM_STRONG_TYPEDEF_LIMITS(To, Min, Max)                                                   \
    namespace std                                                                                  \
    {                                                                                              \
    template <> struct numeric_limits<To>                                                          \
    {                                                                                              \
        static To min() { return To{Min}; }                                                        \
        static To max() { return To{Max}; }                                                        \
    };                                                                                             \
    }
}

#endif // OSRM_STRONG_TYPEDEF_HPP
