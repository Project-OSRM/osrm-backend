#ifndef MERCATOR_HPP
#define MERCATOR_HPP

namespace osrm
{
namespace util
{

struct mercator
{
    static double y2lat(const double value) noexcept;

    static double lat2y(const double latitude) noexcept;
};

}
}

#endif // MERCATOR_HPP
