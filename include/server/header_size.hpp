/*

Copyright (c) 2026, Project OSRM contributors
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

#ifndef SERVER_HEADER_SIZE_HPP
#define SERVER_HEADER_SIZE_HPP

#include "engine/engine_config.hpp"

#include <algorithm>

namespace osrm::server
{

inline unsigned deriveMaxHeaderSize(const engine::EngineConfig &config)
{
    constexpr unsigned MIN_HEADER_SIZE = 8 * 1024;

    int max_coordinates = 0;
    max_coordinates = std::max(max_coordinates, config.max_locations_trip);
    max_coordinates = std::max(max_coordinates, config.max_locations_viaroute);
    max_coordinates = std::max(max_coordinates, config.max_locations_distance_table);
    max_coordinates = std::max(max_coordinates, config.max_locations_map_matching);

    // We estimate the maximum GET line length in bytes:
    // coordinates          | (2-3 (major) + 1 (dot) + 6 (decimals)) * 2 + 1 semi-colon = 21 chars
    // sources/destinations | 4 (digits) + 1 semi-colon = 5 chars
    // approaches           | 12 (chars) + 1 semi-colon = 13 chars
    // radius               | 3 (digits) + 1 semi-colon = 4 chars
    // Total of 47 chars and we add a generous 1024 chars for the request line
    return std::max(MIN_HEADER_SIZE, 48 * static_cast<unsigned>(max_coordinates) + 1024u);
}

} // namespace osrm::server

#endif // SERVER_HEADER_SIZE_HPP
