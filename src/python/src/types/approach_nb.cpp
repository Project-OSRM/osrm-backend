#include "python/types/approach_nb.hpp"
#include "python/utility/param_utility.hpp"
#include "engine/approach.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

NB_MAKE_OPAQUE(osrm::engine::Approach)

namespace nb = nanobind;

void init_Approach(nb::module_ &m)
{
    using osrm::engine::Approach;

    nb::class_<Approach>(m, "Approach")
        .def("__init__",
             [](Approach *t, const std::string &str)
             {
                 Approach approach = osrm_nb_util::str_to_enum(str, "Approach", approach_map);
                 new (t) Approach(approach);
             })
        .def("__repr__",
             [](Approach type)
             { return osrm_nb_util::enum_to_str(type, "Approach", approach_map); });
    nb::implicitly_convertible<std::string, Approach>();
}
