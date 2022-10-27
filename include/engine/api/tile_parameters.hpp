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

#ifndef ENGINE_API_TILE_PARAMETERS_HPP
#define ENGINE_API_TILE_PARAMETERS_HPP

#include <cmath>

namespace osrm
{
namespace engine
{
namespace api
{

/**
 * Parameters specific to the OSRM Tile service.
 *
 * Holds member attributes:
 *  - x: the x location for the tile
 *  - y: the y location for the tile
 *  - z: the zoom level for the tile
 *
 * The parameters x,y and z have to conform to the Slippy Map Tilenames specification:
 *  - https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Zoom_levels
 *  - https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#X_and_Y
 *
 * \see OSRM, Coordinate, Hint, Bearing, RouteParame, RouteParameters, TableParameters,
 *      NearestParameters, TripParameters, MatchParameters and TileParameters
 */
struct TileParameters final
{
    unsigned x;
    unsigned y;
    unsigned z;

    bool IsValid() const
    {
        // https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Zoom_levels
        // https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#X_and_Y
        const auto valid_x = x <= static_cast<unsigned>(std::pow(2., z)) - 1;
        const auto valid_y = y <= static_cast<unsigned>(std::pow(2., z)) - 1;
        // zoom limits are due to slippy map and server performance limits
        const auto valid_z = z < 20 && z >= 12;

        return valid_x && valid_y && valid_z;
    }
};
} // namespace api
} // namespace engine
} // namespace osrm

#endif
