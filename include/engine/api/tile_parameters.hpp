#ifndef ENGINE_API_TILE_PARAMETERS_HPP
#define ENGINE_API_TILE_PARAMETERS_HPP

namespace osrm
{
namespace engine
{
namespace api
{

struct TileParameters final
{
    unsigned x;
    unsigned y;
    unsigned z;

    // FIXME check if x and y work with z
    bool IsValid()
    {
        return z < 20;
    };
};

}
}
}

#endif
