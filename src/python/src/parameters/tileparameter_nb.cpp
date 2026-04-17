#include "python/parameters/tileparameter_nb.hpp"
#include "engine/api/tile_parameters.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <stdexcept>

namespace nb = nanobind;
using namespace nb::literals;

void init_TileParameters(nb::module_ &m)
{
    using osrm::engine::api::TileParameters;

    nb::class_<TileParameters>(m, "TileParameters", nb::is_final())
        .def(nb::init<>(),
             "Instantiates an instance of TileParameters.\n\n"
             "Examples:\n\
                >>> tile_params = osrm.TileParameters([17059, 11948, 15])\n\
                >>> tile_params = osrm.TileParameters(\n\
                        x = 17059,\n\
                        y = 11948,\n\
                        z = 15\n\
                    )\n\
                >>> tile_params.IsValid()\n\
                True\n\n"
             "Args:\n\
                list (list of int): Instantiates an instance of TileParameters using an array [x, y, z].\n\
                x (int): x value.\n\
                y (int): y value.\n\
                z (int): z value.\n\n"
             "Returns:\n\
                __init__ (osrm.TileParameters): A TileParameters object, for usage in Tile.\n\
                IsValid (bool): A bool value denoting validity of parameter values.\n\n"
             "Attributes:\n\
                x (int): x value.\n\
                y (int): y value.\n\
                z (int): z value.")
        .def(nb::init<unsigned int, unsigned int, unsigned int>())
        .def("__init__",
             [](TileParameters *t, const std::vector<unsigned int> &coord)
             {
                 if (coord.size() != 3)
                 {
                     throw std::runtime_error("Parameter must be an array [x, y, z]");
                 }

                 new (t) TileParameters{coord[0], coord[1], coord[2]};
             })
        .def_rw("x", &TileParameters::x)
        .def_rw("y", &TileParameters::y)
        .def_rw("z", &TileParameters::z)
        .def("IsValid", &TileParameters::IsValid);
    nb::implicitly_convertible<std::vector<unsigned int>, TileParameters>();
}
