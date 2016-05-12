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

#ifndef ENGINE_HINT_HPP
#define ENGINE_HINT_HPP

#include "engine/phantom_node.hpp"

#include "util/coordinate.hpp"

#include <cstdint>
#include <iosfwd>
#include <string>

namespace osrm
{
namespace engine
{

// Fwd. decls.
namespace datafacade
{
class BaseDataFacade;
}

// Is returned as a temporary identifier for snapped coodinates
struct Hint
{
    PhantomNode phantom;
    std::uint32_t data_checksum;

    bool IsValid(const util::Coordinate new_input_coordinates,
                 const datafacade::BaseDataFacade &facade) const;

    std::string ToBase64() const;
    static Hint FromBase64(const std::string &base64Hint);

    friend bool operator==(const Hint &, const Hint &);
    friend std::ostream &operator<<(std::ostream &, const Hint &);
};

static_assert(sizeof(Hint) == 64 + 4, "Hint is bigger than expected");
constexpr std::size_t ENCODED_HINT_SIZE = 92;
static_assert(ENCODED_HINT_SIZE / 4 * 3 >= sizeof(Hint),
              "ENCODED_HINT_SIZE does not match size of Hint");
}
}

#endif
