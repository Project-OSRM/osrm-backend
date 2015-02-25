/*

Copyright (c) 2015, Project OSRM, Dennis Luxen, others
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

#ifndef PBF_RENDERER_HPP
#define PBF_RENDERER_HPP

#include "cast.hpp"

#include <osrm/json_container.hpp>

#include <type_traits>

namespace JSON {

struct PBFToArrayRenderer : mapbox::util::static_visitor<>
{
    explicit PBFToArrayRenderer(std::vector<char> &_out) : out(_out) {}

    void operator()(const String &string) const
    {
        out.insert(out.end(), string.value.begin(), string.value.end());
    }

    template<class Other>
    typename std::enable_if<!std::is_same<Other, String>::value, void>::type
    operator()(const Other &other) const
    {
    }

  private:
    std::vector<char> &out;
};

template<class JSONObject>
void pbf_render(std::vector<char> &out, const JSONObject &object)
{
    Value value = object;
    mapbox::util::apply_visitor(PBFToArrayRenderer(out), value);
}

} // namespace JSON

#endif // PBF_RENDERER_HPP
