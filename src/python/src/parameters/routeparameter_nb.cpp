#include "python/parameters/routeparameter_nb.hpp"
#include "python/utility/param_utility.hpp"
#include "engine/api/route_parameters.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using namespace nb::literals;

void init_RouteParameters(nb::module_ &m)
{
    using osrm::engine::api::BaseParameters;
    using osrm::engine::api::RouteParameters;

    nb::class_<RouteParameters, BaseParameters>(m, "RouteParameters")
        .def(nb::init<>(),
             "Instantiates an instance of RouteParameters.\n\n"
             "Examples:\n\
                >>> route_params = osrm.RouteParameters(\n\
                        coordinates = [(7.41337, 43.72956), (7.41546, 43.73077)],\n\
                        steps = True,\n\
                        number_of_alternatives = 3,\n\
                        annotations = ['speed'],\n\
                        geometries = 'polyline',\n\
                        overview = 'simplified',\n\
                        continue_straight = False,\n\
                        waypoints = [0, 1],\n\
                        radiuses = [4.07, 4.07],\n\
                        bearings = [(200, 180), (250, 180)],\n\
                        # approaches = ['unrestricted', 'unrestricted'],\n\
                        generate_hints = False,\n\
                        exclude = ['motorway'],\n\
                        snapping = 'any'\n\
                    )\n\
                >>> route_params.IsValid()\n\
                True\n\n"
             "Args:\n\
                steps (bool): Return route steps for each route leg. (default False)\n\
                number_of_alternatives (int): Search for n alternative routes. (default 0)\n\
                annotations (list of 'none' | 'duration' |  'nodes' | 'distance' | 'weight' | 'datasources' \
                    | 'speed' | 'all'): Returns additional metadata for each coordinate along the route geometry. (default [])\n\
                geometries (string 'polyline' | 'polyline6' | 'geojson'): Returned route geometry format - influences overview and per step. (default "
             ")\n\
                overview (string 'simplified' | 'full' | 'false'): Add overview geometry either full, simplified. (default '')\n\
                continue_straight (bool): Forces the route to keep going straight at waypoints, constraining u-turns. (default {})\n\
                waypoints (list of int): Treats input coordinates indicated by given indices as waypoints in returned Match object. (default [])\n\
                BaseParameters (osrm.osrm_ext.BaseParameters): Keyword arguments from parent class.\n\n"
             "Returns:\n\
                __init__ (osrm.RouteParameters): A RouteParameters object, for usage in Route.\n\
                IsValid (bool): A bool value denoting validity of parameter values.\n\n"
             "Attributes:\n\
                steps (bool): Return route steps for each route leg.\n\
                alternatives (bool): Search for alternative routes.\n\
                number_of_alternatives (int): Search for n alternative routes.\n\
                annotations_type (string): Returns additional metadata for each coordinate along the route geometry.\n\
                geometries (string): Returned route geometry format - influences overview and per step.\n\
                overview (string): Add overview geometry either full, simplified.\n\
                continue_straight (bool): Forces the route to keep going straight at waypoints, constraining u-turns.\n\
                BaseParameters (osrm.osrm_ext.BaseParameters): Attributes from parent class.")
        .def(
            "__init__",
            [](RouteParameters *t,
               const bool steps,
               int number_of_alternatives,
               const std::vector<RouteParameters::AnnotationsType> &annotations,
               RouteParameters::GeometriesType geometries,
               RouteParameters::OverviewType overview,
               const std::optional<bool> continue_straight,
               std::vector<std::size_t> waypoints,
               std::vector<osrm::util::Coordinate> coordinates,
               std::vector<std::optional<std::string>> hints,
               std::vector<std::optional<double>> radiuses,
               std::vector<std::optional<osrm::engine::Bearing>> bearings,
               const std::vector<std::optional<osrm::engine::Approach>> &approaches,
               bool generate_hints,
               std::vector<std::string> exclude,
               const BaseParameters::SnappingType snapping)
            {
                new (t) RouteParameters();

                osrm_nb_util::assign_routeparameters(t,
                                                     steps,
                                                     number_of_alternatives,
                                                     annotations,
                                                     geometries,
                                                     overview,
                                                     continue_straight,
                                                     waypoints);

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
            "steps"_a = false,
            "number_of_alternatives"_a = 0,
            "annotations"_a = std::vector<std::string>(),
            "geometries"_a = std::string(),
            "overview"_a = std::string(),
            "continue_straight"_a = std::optional<bool>(),
            "waypoints"_a = std::vector<std::size_t>(),
            "coordinates"_a = std::vector<osrm::util::Coordinate>(),
            "hints"_a = std::vector<std::optional<std::string>>(),
            "radiuses"_a = std::vector<std::optional<double>>(),
            "bearings"_a = std::vector<std::optional<osrm::engine::Bearing>>(),
            "approaches"_a = std::vector<std::string *>(),
            "generate_hints"_a = true,
            "exclude"_a = std::vector<std::string>(),
            "snapping"_a = std::string())
        .def_rw("steps", &RouteParameters::steps)
        .def_rw("alternatives", &RouteParameters::alternatives)
        .def_rw("number_of_alternatives", &RouteParameters::number_of_alternatives)
        .def_rw("annotations_type", &RouteParameters::annotations_type)
        .def_rw("geometries", &RouteParameters::geometries)
        .def_rw("overview", &RouteParameters::overview)
        .def_rw("continue_straight", &RouteParameters::continue_straight)
        .def("IsValid", &RouteParameters::IsValid);

    nb::class_<RouteParameters::GeometriesType>(m, "RouteGeometriesType")
        .def(
            "__init__",
            [](RouteParameters::GeometriesType *t, const std::string &str)
            {
                RouteParameters::GeometriesType geometries =
                    osrm_nb_util::str_to_enum(str, "RouteGeometriesType", geometries_map);
                new (t) RouteParameters::GeometriesType(geometries);
            },
            "Instantiates a GeometriesType based on provided String value.")
        .def(
            "__repr__",
            [](RouteParameters::GeometriesType type)
            { return osrm_nb_util::enum_to_str(type, "RouteGeometriesType", geometries_map); },
            "Return a String based on GeometriesType value.");
    nb::implicitly_convertible<std::string, RouteParameters::GeometriesType>();

    nb::class_<RouteParameters::OverviewType>(m, "RouteOverviewType")
        .def(
            "__init__",
            [](RouteParameters::OverviewType *t, const std::string &str)
            {
                RouteParameters::OverviewType overview =
                    osrm_nb_util::str_to_enum(str, "RouteOverviewType", overview_map);
                new (t) RouteParameters::OverviewType(overview);
            },
            "Instantiates a OverviewType based on provided String value.")
        .def(
            "__repr__",
            [](RouteParameters::OverviewType type)
            { return osrm_nb_util::enum_to_str(type, "RouteOverviewType", overview_map); },
            "Return a String based on OverviewType value.");
    nb::implicitly_convertible<std::string, RouteParameters::OverviewType>();

    nb::class_<RouteParameters::AnnotationsType>(m, "RouteAnnotationsType")
        .def(
            "__init__",
            [](RouteParameters::AnnotationsType *t, const std::string &str)
            {
                RouteParameters::AnnotationsType annotation =
                    osrm_nb_util::str_to_enum(str, "RouteAnnotationsType", route_annotations_map);
                new (t) RouteParameters::AnnotationsType(annotation);
            },
            "Instantiates a AnnotationsType based on provided String value.")
        .def(
            "__repr__",
            [](RouteParameters::AnnotationsType type) { return std::to_string((int)type); },
            "Return a String based on AnnotationsType value.")
        .def(
            "__and__",
            [](RouteParameters::AnnotationsType lhs, RouteParameters::AnnotationsType rhs)
            { return lhs & rhs; },
            nb::is_operator(),
            "Return the bitwise AND result of two AnnotationsTypes.")
        .def(
            "__or__",
            [](RouteParameters::AnnotationsType lhs, RouteParameters::AnnotationsType rhs)
            { return lhs | rhs; },
            nb::is_operator(),
            "Return the bitwise OR result of two AnnotationsTypes.")
        .def(
            "__ior__",
            [](RouteParameters::AnnotationsType &lhs, RouteParameters::AnnotationsType rhs)
            { return lhs = lhs | rhs; },
            nb::is_operator(),
            "Add the bitwise OR value of another AnnotationsType.");
    nb::implicitly_convertible<std::string, RouteParameters::AnnotationsType>();
}
