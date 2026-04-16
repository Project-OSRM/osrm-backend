#include "python/parameters/baseparameter_nb.hpp"
#include "python/utility/param_utility.hpp"
#include "engine/api/base_parameters.hpp"
#include "engine/hint.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using namespace nb::literals;

void init_BaseParameters(nb::module_ &m)
{
    using osrm::engine::api::BaseParameters;

    nb::class_<BaseParameters>(m, "BaseParameters")
        .def(nb::init<>(),
             "Instantiates an instance of BaseParameters.\n\n"
             "Note:\n\
                This is the parent class to many parameter classes, and not intended to be used on its own.\n\n"
             "Args:\n\
                coordinates (list of floats pairs): Pairs of Longitude and Latitude Coordinates. (default [])\n\
                hints (list): Hint from previous request to derive position in street network. (default [])\n\
                radiuses (list of floats): Limits the search to given radius in meters. (default [])\n\
                bearings (list of int pairs): Limits the search to segments with given bearing in degrees towards true north in clockwise direction. (default [])\n\
                approaches (list): Keep waypoints on curb side. (default [])\n\
                generate_hints (bool): Adds a hint to the response which can be used in subsequent requests. (default True)\n\
                exclude (list of strings): Additive list of classes to avoid. (default [])\n\
                snapping (string 'default' | 'any'): 'default' snapping avoids is_startpoint edges, 'any' will snap to any edge in the graph. (default '')\n\n"
             "Returns:\n\
                __init__ (osrm.osrm_ext.BaseParameters): A BaseParameter object, that is the parent object to many other Parameter objects.\n\
                IsValid (bool): A bool value denoting validity of parameter values.\n\n"
             "Attributes:\n\
                coordinates (list of floats pairs): Pairs of longitude & latitude coordinates.\n\
                hints (list): Hint from previous request to derive position in street network.\n\
                radiuses (list of floats): Limits the search to given radius in meters.\n\
                bearings (list of int pairs): Limits the search to segments with given bearing in degrees towards true north in clockwise direction.\n\
                approaches (list): Keep waypoints on curb side.\n\
                exclude (list of strings): Additive list of classes to avoid, order does not matter.\n\
                format (string): Specifies response type - currently only 'json' is supported.\n\
                generate_hints (bool): Adds a hint to the response which can be used in subsequent requests.\n\
                skip_waypoints (list): Removes waypoints from the response.\n\
                snapping (string): 'default' snapping avoids is_startpoint edges, 'any' will snap to any edge in the graph.")
        .def_rw("coordinates", &BaseParameters::coordinates)
        .def_prop_rw(
            "hints",
            [](const BaseParameters &p)
            {
                nb::list result;
                for (const auto &h : p.hints)
                {
                    if (h)
                    {
                        result.append(h->ToBase64());
                    }
                    else
                    {
                        result.append(nb::none());
                    }
                }
                return result;
            },
            [](BaseParameters &p, const nb::list &hints)
            {
                p.hints.clear();
                for (auto item : hints)
                {
                    if (item.is_none())
                    {
                        p.hints.push_back(std::nullopt);
                    }
                    else
                    {
                        p.hints.push_back(
                            osrm::engine::Hint::FromBase64(nb::cast<std::string>(item)));
                    }
                }
            })
        .def_rw("radiuses", &BaseParameters::radiuses)
        .def_rw("bearings", &BaseParameters::bearings)
        .def_rw("approaches", &BaseParameters::approaches)
        .def_rw("exclude", &BaseParameters::exclude)
        .def_rw("format", &BaseParameters::format)
        .def_rw("generate_hints", &BaseParameters::generate_hints)
        .def_rw("skip_waypoints", &BaseParameters::skip_waypoints)
        .def_rw("snapping", &BaseParameters::snapping)
        .def("IsValid", &BaseParameters::IsValid);

    nb::class_<BaseParameters::SnappingType>(m, "SnappingType")
        .def(
            "__init__",
            [](BaseParameters::SnappingType *t, const std::string &str)
            {
                BaseParameters::SnappingType snapping =
                    osrm_nb_util::str_to_enum(str, "SnappingType", snapping_map);
                new (t) BaseParameters::SnappingType(snapping);
            },
            "Instantiates a SnappingType based on provided String value.")
        .def(
            "__repr__",
            [](BaseParameters::SnappingType type)
            { return osrm_nb_util::enum_to_str(type, "SnappingType", snapping_map); },
            "Return a String based on SnappingType value.");
    nb::implicitly_convertible<std::string, BaseParameters::SnappingType>();

    nb::class_<BaseParameters::OutputFormatType>(m, "OutputFormatType")
        .def(
            "__init__",
            [](BaseParameters::OutputFormatType *t, const std::string &str)
            {
                BaseParameters::OutputFormatType output =
                    osrm_nb_util::str_to_enum(str, "OutputFormatType", output_map);
                new (t) BaseParameters::OutputFormatType(output);
            },
            "Instantiates a OutputFormatType based on provided String value.")
        .def(
            "__repr__",
            [](BaseParameters::OutputFormatType type)
            { return osrm_nb_util::enum_to_str(type, "OutputFormatType", output_map); },
            "Return a String based on OutputFormatType value.");
    nb::implicitly_convertible<std::string, BaseParameters::OutputFormatType>();
}
