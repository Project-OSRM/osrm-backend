#include "python/parameters/tableparameter_nb.hpp"
#include "python/parameters/baseparameter_nb.hpp"
#include "python/utility/param_utility.hpp"
#include "engine/api/table_parameters.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using namespace nb::literals;

void init_TableParameters(nb::module_ &m)
{
    using osrm::engine::api::BaseParameters;
    using osrm::engine::api::TableParameters;
    static const std::unordered_map<std::string, TableParameters::AnnotationsType>
        table_annotations_map{{"none", TableParameters::AnnotationsType::None},
                              {std::string(), TableParameters::AnnotationsType::None},
                              {"duration", TableParameters::AnnotationsType::Duration},
                              {"distance", TableParameters::AnnotationsType::Distance},
                              {"all", TableParameters::AnnotationsType::All}};

    nb::class_<TableParameters, BaseParameters>(m, "TableParameters")
        .def(nb::init<>(),
             "Instantiates an instance of TableParameters.\n\n"
             "Examples:\n\
                >>> table_params = osrm.TableParameters(\n\
                        coordinates = [(7.41337, 43.72956), (7.41546, 43.73077)],\n\
                        sources = [0],\n\
                        destinations = [1],\n\
                        annotations = ['duration'],\n\
                        fallback_speed = 1,\n\
                        fallback_coordinate_type = 'input',\n\
                        scale_factor = 0.9\n\
                    )\n\
                >>> table_params.IsValid()\n\
                True\n\n"
             "Args:\n\
                sources (list of int): Use location with given index as source. (default [])\n\
                destinations (list of int): Use location with given index as destination. (default [])\n\
                annotations (list of 'none' | 'duration' | 'distance' | 'all'): \
                    Returns additional metadata for each coordinate along the route geometry. (default [])\n\
                fallback_speed (float): If no route found between a source/destination pair, calculate the as-the-crow-flies distance, \
                    then use this speed to estimate duration. (default INVALID_FALLBACK_SPEED)\n\
                fallback_coordinate_type (string 'input' | 'snapped'): When using a fallback_speed, use the user-supplied coordinate (input), \
                    or the snapped location (snapped) for calculating distances. (default '')\n\
                scale_factor: Scales the table duration values by this number (use in conjunction with annotations=durations). (default 1.0)\n\
                BaseParameters (osrm.osrm_ext.BaseParameters): Keyword arguments from parent class.\n\n"
             "Returns:\n\
                __init__ (osrm.TableParameters): A TableParameters object, for usage in Table.\n\
                IsValid (bool): A bool value denoting validity of parameter values.\n\n"
             "Attributes:\n\
                sources (list of int): Use location with given index as source.\n\
                destinations (list of int): Use location with given index as destination.\n\
                annotations (string): Returns additional metadata for each coordinate along the route geometry.\n\
                fallback_speed (float): If no route found between a source/destination pair, calculate the as-the-crow-flies distance, \
                    then use this speed to estimate duration.\n\
                fallback_coordinate_type (string): When using a fallback_speed, use the user-supplied coordinate (input), \
                    or the snapped location (snapped) for calculating distances.\n\
                scale_factor: Scales the table duration values by this number (use in conjunction with annotations=durations).\n\
                BaseParameters (osrm.osrm_ext.BaseParameters): Attributes from parent class.")
        .def(
            "__init__",
            [](TableParameters *t,
               std::vector<std::size_t> sources,
               std::vector<std::size_t> destinations,
               const std::vector<TableParameters::AnnotationsType> &annotations,
               double fallback_speed,
               TableParameters::FallbackCoordinateType fallback_coordinate_type,
               double scale_factor,
               std::vector<osrm::util::Coordinate> coordinates,
               std::vector<std::optional<std::string>> hints,
               std::vector<std::optional<double>> radiuses,
               std::vector<std::optional<osrm::engine::Bearing>> bearings,
               const std::vector<std::optional<osrm::engine::Approach>> &approaches,
               bool generate_hints,
               std::vector<std::string> exclude,
               const BaseParameters::SnappingType snapping)
            {
                new (t) TableParameters();

                t->sources = std::move(sources);
                t->destinations = std::move(destinations);
                t->annotations = osrm_nb_util::calculate_tableannotations_type(annotations);
                t->fallback_speed = fallback_speed;
                t->fallback_coordinate_type = fallback_coordinate_type;
                t->scale_factor = scale_factor;

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
            "sources"_a = std::vector<std::size_t>(),
            "destinations"_a = std::vector<std::size_t>(),
            "annotations"_a = std::vector<std::string>(),
            "fallback_speed"_a = osrm::from_alias<double>(INVALID_FALLBACK_SPEED),
            "fallback_coordinate_type"_a = std::string(),
            "scale_factor"_a = 1.0,
            "coordinates"_a = std::vector<osrm::util::Coordinate>(),
            "hints"_a = std::vector<std::optional<std::string>>(),
            "radiuses"_a = std::vector<std::optional<double>>(),
            "bearings"_a = std::vector<std::optional<osrm::engine::Bearing>>(),
            "approaches"_a = std::vector<std::string *>(),
            "generate_hints"_a = true,
            "exclude"_a = std::vector<std::string>(),
            "snapping"_a = std::string())
        .def_rw("sources", &TableParameters::sources)
        .def_rw("destinations", &TableParameters::destinations)
        .def_rw("fallback_speed", &TableParameters::fallback_speed)
        .def_rw("fallback_coordinate_type", &TableParameters::fallback_coordinate_type)
        .def_rw("annotations", &TableParameters::annotations)
        .def_rw("scale_factor", &TableParameters::scale_factor)
        .def("IsValid", &TableParameters::IsValid);

    nb::class_<TableParameters::FallbackCoordinateType>(m, "TableFallbackCoordinateType")
        .def(
            "__init__",
            [](TableParameters::FallbackCoordinateType *t, const std::string &str)
            {
                TableParameters::FallbackCoordinateType fallback =
                    osrm_nb_util::str_to_enum(str, "TableFallbackCoordinateType", fallback_map);
                new (t) TableParameters::FallbackCoordinateType(fallback);
            },
            "Instantiates a FallbackCoordinateType based on provided String value.")
        .def(
            "__repr__",
            [](TableParameters::FallbackCoordinateType type) {
                return osrm_nb_util::enum_to_str(type, "TableFallbackCoordinateType", fallback_map);
            },
            "Return a String based on FallbackCoordinateType value.");
    nb::implicitly_convertible<std::string, TableParameters::FallbackCoordinateType>();

    nb::class_<TableParameters::AnnotationsType>(m, "TableAnnotationsType")
        .def(
            "__init__",
            [](TableParameters::AnnotationsType *t, const std::string &str)
            {
                TableParameters::AnnotationsType annotation =
                    osrm_nb_util::str_to_enum(str, "TableAnnotationsType", table_annotations_map);
                new (t) TableParameters::AnnotationsType(annotation);
            },
            "Instantiates a AnnotationsType based on provided String value.")
        .def(
            "__repr__",
            [](TableParameters::AnnotationsType type) { return std::to_string((int)type); },
            "Return a String based on AnnotationsType value.")
        .def(
            "__and__",
            [](TableParameters::AnnotationsType lhs, TableParameters::AnnotationsType rhs)
            { return lhs & rhs; },
            nb::is_operator(),
            "Return the bitwise AND result of two AnnotationsTypes.")
        .def(
            "__or__",
            [](TableParameters::AnnotationsType lhs, TableParameters::AnnotationsType rhs)
            { return lhs | rhs; },
            nb::is_operator(),
            "Return the bitwise OR result of two AnnotationsTypes.")
        .def(
            "__ior__",
            [](TableParameters::AnnotationsType &lhs, TableParameters::AnnotationsType rhs)
            { return lhs = lhs | rhs; },
            nb::is_operator(),
            "Add the bitwise OR value of another AnnotationsType.");
    nb::implicitly_convertible<std::string, TableParameters::AnnotationsType>();
}
