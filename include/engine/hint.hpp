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

#ifndef ENGINE_HINT_HPP
#define ENGINE_HINT_HPP

#include "engine/phantom_node.hpp"

#include "util/coordinate.hpp"

#include <cstdint>
#include <iosfwd>
#include <string>

namespace osrm::engine
{

// Fwd. decls.
namespace datafacade
{
class BaseDataFacade;
}

// SegmentHint represents an individual segment position that could be used
// as the waypoint for a given input location
struct SegmentHint
{
    PhantomNode phantom;
    std::uint32_t data_checksum;

    bool IsValid(const util::Coordinate new_input_coordinates,
                 const datafacade::BaseDataFacade &facade) const;

    std::string ToBase64() const;
    static SegmentHint FromBase64(const std::string &base64Hint);

    friend bool operator==(const SegmentHint &, const SegmentHint &);
    friend bool operator!=(const SegmentHint &, const SegmentHint &);
    friend std::ostream &operator<<(std::ostream &, const SegmentHint &);
};

// Hint represents the suggested segment positions that could be used
// as the waypoint for a given input location
struct Hint
{
    std::vector<SegmentHint> segment_hints;

    bool IsValid(const util::Coordinate new_input_coordinates,
                 const datafacade::BaseDataFacade &facade) const;

    std::string ToBase64() const;
    static Hint FromBase64(const std::string &base64Hint);
};

static_assert(sizeof(SegmentHint) == 80 + 4, "Hint is bigger than expected");
constexpr std::size_t ENCODED_SEGMENT_HINT_SIZE = 112;
static_assert(ENCODED_SEGMENT_HINT_SIZE / 4 * 3 >= sizeof(SegmentHint),
              "ENCODED_SEGMENT_HINT_SIZE does not match size of SegmentHint");

} // namespace osrm::engine

#endif
