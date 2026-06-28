#include "python/types/bearing_nb.hpp"
#include "engine/bearing.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/pair.h>

namespace nb = nanobind;

void init_Bearing(nb::module_ &m)
{
    using osrm::engine::Bearing;

    nb::class_<Bearing>(m, "Bearing")
        .def(nb::init<>())
        .def("__init__",
             [](Bearing *t, std::pair<int16_t, int16_t> pair)
             { new (t) Bearing{pair.first, pair.second}; })
        .def_rw("bearing", &Bearing::bearing)
        .def_rw("range", &Bearing::range)
        .def("IsValid", &Bearing::IsValid)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self);
    nb::implicitly_convertible<std::pair<int16_t, int16_t>, Bearing>();
}
