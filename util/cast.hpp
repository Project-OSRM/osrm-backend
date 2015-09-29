/*

Copyright (c) 2015, Project OSRM contributors
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

#ifndef CAST_HPP
#define CAST_HPP

#include <string>
#include <sstream>
#include <iomanip>
#include <type_traits>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace cast
{
template <typename Enumeration>
inline auto enum_to_underlying(Enumeration const value) ->
    typename std::underlying_type<Enumeration>::type
{
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

template <typename T, int Precision = 6> inline std::string to_string_with_precision(const T x)
{
    static_assert(std::is_arithmetic<T>::value, "integral or floating point type required");

    std::ostringstream out;
    out << std::fixed << std::setprecision(Precision) << x;
    auto rv = out.str();

    // Javascript has no separation of float / int, digits without a '.' are integral typed
    // X.Y.0 -> X.Y
    // X.0 -> X
    boost::trim_right_if(rv, boost::is_any_of("0"));
    boost::trim_right_if(rv, boost::is_any_of("."));
    // Note:
    //  - assumes the locale to use '.' as digit separator
    //  - this is not identical to:  trim_right_if(rv, is_any_of('0 .'))

    return rv;
}
}

#endif // CAST_HPP
