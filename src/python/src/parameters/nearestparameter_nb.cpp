#include "python/parameters/nearestparameter_nb.hpp"
#include "python/parameters/baseparameter_nb.hpp"
#include "python/utility/param_utility.hpp"
#include "engine/api/nearest_parameters.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using namespace nb::literals;

void init_NearestParameters(nb::module_ &m)
{
    using osrm::engine::api::BaseParameters;
    using osrm::engine::api::NearestParameters;

    nb::class_<NearestParameters, BaseParameters>(m, "NearestParameters")
        .def(nb::init<>(),
             "Instantiates an instance of NearestParameters.\n\n"
             "Examples:\n\
                >>> nearest_params = osrm.NearestParameters(\n\
                        coordinates = [(7.41337, 43.72956)],\n\
                        exclude = ['motorway']\n\
                    )\n\
                >>> nearest_params.IsValid()\n\
                True\n\n"
             "Args:\n\
                BaseParameters (osrm.osrm_ext.BaseParameters): Keyword arguments from parent class.\n\n"
             "Returns:\n\
                __init__ (osrm.NearestParameters): A NearestParameters object, for usage in osrm.OSRM.Nearest.\n\
                IsValid (bool): A bool value denoting validity of parameter values.\n\n"
             "Attributes:\n\
                number_of_results (unsigned int): Number of nearest segments that should be returned.\n\
                BaseParameters (osrm.osrm_ext.BaseParameters): Attributes from parent class.")
        .def(
            "__init__",
            [](NearestParameters *t,
               std::vector<osrm::util::Coordinate> coordinates,
               std::vector<std::optional<std::string>> hints,
               std::vector<std::optional<double>> radiuses,
               std::vector<std::optional<osrm::engine::Bearing>> bearings,
               const std::vector<std::optional<osrm::engine::Approach>> &approaches,
               bool generate_hints,
               std::vector<std::string> exclude,
               const BaseParameters::SnappingType snapping)
            {
                new (t) NearestParameters();

                osrm_nb_util::assign_baseparameters(t,
                                                    coordinates,
                                                    hints,
                                                    radiuses,
                                                    bearings,
                                                    approaches,
                                                    generate_hints,
                                                    exclude,
                                                    snapping);
            },
            "coordinates"_a = std::vector<osrm::util::Coordinate>(),
            "hints"_a = std::vector<std::optional<std::string>>(),
            "radiuses"_a = std::vector<std::optional<double>>(),
            "bearings"_a = std::vector<std::optional<osrm::engine::Bearing>>(),
            "approaches"_a = std::vector<std::string *>(),
            "generate_hints"_a = true,
            "exclude"_a = std::vector<std::string>(),
            "snapping"_a = std::string())
        .def_rw("number_of_results", &NearestParameters::number_of_results)
        .def("IsValid", &NearestParameters::IsValid);
}
