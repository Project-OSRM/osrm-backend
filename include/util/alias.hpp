/*

Copyright (c) 2017, Project OSRM contributors
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

#ifndef OSRM_UTIL_ALIAS_HPP
#define OSRM_UTIL_ALIAS_HPP

#include <functional>
#include <iostream>
#include <type_traits>

namespace osrm
{

template <typename From, typename Tag> struct Alias;
template <typename From, typename Tag>
inline std::ostream &operator<<(std::ostream &stream, const Alias<From, Tag> &inst);

template <typename From, typename Tag> struct Alias final
{
    using value_type = From;
    static_assert(std::is_arithmetic<From>::value, "Needs to be based on an arithmetic type");

    From __value;
    friend std::ostream &operator<<<From, Tag>(std::ostream &stream, const Alias &inst);

    explicit operator From &() { return __value; }
    explicit operator From() const { return __value; }
    inline Alias operator+(const Alias rhs_) const
    {
        return Alias{__value + static_cast<const From>(rhs_)};
    }
    inline Alias operator-(const Alias rhs_) const
    {
        return Alias{__value - static_cast<const From>(rhs_)};
    }
    inline Alias operator*(const Alias rhs_) const
    {
        return Alias{__value * static_cast<const From>(rhs_)};
    }
    inline Alias operator*(const double rhs_) const { return Alias{__value * rhs_}; }
    inline Alias operator/(const Alias rhs_) const
    {
        return Alias{__value / static_cast<const From>(rhs_)};
    }
    inline Alias operator/(const double rhs_) const { return Alias{__value / rhs_}; }
    inline Alias operator|(const Alias rhs_) const
    {
        return Alias{__value | static_cast<const From>(rhs_)};
    }
    inline Alias operator&(const Alias rhs_) const
    {
        return Alias{__value & static_cast<const From>(rhs_)};
    }
    inline bool operator<(const Alias z_) const { return __value < static_cast<const From>(z_); }
    inline bool operator>(const Alias z_) const { return __value > static_cast<const From>(z_); }
    inline bool operator<=(const Alias z_) const { return __value <= static_cast<const From>(z_); }
    inline bool operator>=(const Alias z_) const { return __value >= static_cast<const From>(z_); }
    inline bool operator==(const Alias z_) const { return __value == static_cast<const From>(z_); }
    inline bool operator!=(const Alias z_) const { return __value != static_cast<const From>(z_); }

    inline Alias operator++()
    {
        __value++;
        return *this;
    }
    inline Alias operator--()
    {
        __value--;
        return *this;
    }

    inline Alias operator+=(const Alias z_)
    {
        __value += static_cast<const From>(z_);
        return *this;
    }
    inline Alias operator-=(const Alias z_)
    {
        __value -= static_cast<const From>(z_);
        return *this;
    }
    inline Alias operator/=(const Alias z_)
    {
        __value /= static_cast<const From>(z_);
        return *this;
    }
    inline Alias operator*=(const Alias z_)
    {
        __value *= static_cast<const From>(z_);
        return *this;
    }
    inline Alias operator|=(const Alias z_)
    {
        __value |= static_cast<const From>(z_);
        return *this;
    }
    inline Alias operator&=(const Alias z_)
    {
        __value &= static_cast<const From>(z_);
        return *this;
    }
};

template <typename From, typename Tag>
inline std::ostream &operator<<(std::ostream &stream, const Alias<From, Tag> &inst)
{
    return stream << inst.__value;
}
} // namespace osrm

namespace std
{
template <typename From, typename Tag> struct hash<osrm::Alias<From, Tag>>
{
    typedef osrm::Alias<From, Tag> argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const &s) const
    {
        return std::hash<From>()(static_cast<const From>(s));
    }
};
} // namespace std

#endif // OSRM_ALIAS_HPP
