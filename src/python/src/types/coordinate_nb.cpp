#include "python/types/coordinate_nb.hpp"
#include "util/coordinate.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>

#include <string>

namespace nb = nanobind;
using namespace nb::literals;

void init_Coordinate(nb::module_ &m)
{
    namespace tag = osrm::util::tag;
    using FloatLongitude = osrm::Alias<double, tag::longitude>;
    using FloatLatitude = osrm::Alias<double, tag::latitude>;

    using osrm::util::Coordinate;

    nb::class_<Coordinate>(m, "Coordinate")
        .def(nb::init<>())
        .def(nb::init<const Coordinate &>(), "coordinate"_a)
        .def("__init__",
             [](Coordinate *t, std::pair<double, double> coords)
             {
                 const FloatLongitude lon_ = FloatLongitude{coords.first};
                 const FloatLatitude lat_ = FloatLatitude{coords.second};

                 new (t) Coordinate(lon_, lat_);
             })
        .def_prop_rw(
            "lon",
            [](const Coordinate &c)
            { return static_cast<double>(static_cast<int>(c.lon)) / osrm::COORDINATE_PRECISION; },
            [](Coordinate &c, double val) { c.lon = osrm::util::toFixed(FloatLongitude{val}); })
        .def_prop_rw(
            "lat",
            [](const Coordinate &c)
            { return static_cast<double>(static_cast<int>(c.lat)) / osrm::COORDINATE_PRECISION; },
            [](Coordinate &c, double val) { c.lat = osrm::util::toFixed(FloatLatitude{val}); })
        .def("IsValid", &Coordinate::IsValid)
        .def("__repr__",
             [](const Coordinate &coord)
             {
                 int lon = static_cast<int>(coord.lon);
                 int lat = static_cast<int>(coord.lat);

                 return '(' + std::to_string(lon) + ',' + std::to_string(lat) + ')';
             })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self);
    nb::implicitly_convertible<std::pair<double, double>, Coordinate>();
}
