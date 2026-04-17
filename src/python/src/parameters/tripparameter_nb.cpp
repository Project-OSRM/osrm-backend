#include "python/parameters/tripparameter_nb.hpp"
#include "python/parameters/routeparameter_nb.hpp"
#include "python/utility/param_utility.hpp"
#include "engine/api/trip_parameters.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using namespace nb::literals;

void init_TripParameters(nb::module_ &m)
{
    using osrm::engine::api::RouteParameters;
    using osrm::engine::api::TripParameters;

    nb::class_<TripParameters, RouteParameters>(m, "TripParameters")
        .def(nb::init<>(),
             "Instantiates an instance of TripParameters.\n\n"
             "Examples:\n\
                >>> trip_params = osrm.TripParameters(\n\
                        coordinates = [(7.41337, 43.72956), (7.41546, 43.73077)],\n\
                        source = 'any',\n\
                        destination = 'last',\n\
                        roundtrip = False\n\
                    )\n\
                >>> trip_params.IsValid()\n\
                True\n\n"
             "Args:\n\
                source (string 'any' | 'first'): Returned route starts at 'any' or 'first' coordinate. (default '')\n\
                destination (string 'any' | 'last'): Returned route ends at 'any' or 'last' coordinate. (default '')\n\
                roundtrip (bool): Returned route is a roundtrip (route returns to first location). (default True)\n\
                RouteParameters (osrm.RouteParameters): Keyword arguments from parent class.\n\n"
             "Returns:\n\
                __init__ (osrm.TripParameters): A TripParameters object, for usage in Trip.\n\
                IsValid (bool): A bool value denoting validity of parameter values.\n\n"
             "Attributes:\n\
                source (string): Returned route starts at 'any' or 'first' coordinate.\n\
                destination (string): Returned route ends at 'any' or 'last' coordinate.\n\
                roundtrip (bool): Returned route is a roundtrip (route returns to first location).\n\
                RouteParameters (osrm.RouteParameters): Attributes from parent class.")
        .def(
            "__init__",
            [](TripParameters *t,
               TripParameters::SourceType source,
               TripParameters::DestinationType destination,
               bool roundtrip,
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
                new (t) TripParameters();

                t->source = source;
                t->destination = destination;
                t->roundtrip = roundtrip;

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
            "source"_a = std::string(),
            "destination"_a = std::string(),
            "roundtrip"_a = true,
            "steps"_a = false,
            "alternatives"_a = 0,
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
        .def_rw("source", &TripParameters::source)
        .def_rw("destination", &TripParameters::destination)
        .def_rw("roundtrip", &TripParameters::roundtrip)
        .def("IsValid", &TripParameters::IsValid);

    nb::class_<TripParameters::SourceType>(m, "TripSourceType")
        .def(
            "__init__",
            [](TripParameters::SourceType *t, const std::string &str)
            {
                TripParameters::SourceType source =
                    osrm_nb_util::str_to_enum(str, "TripSourceType", source_map);
                new (t) TripParameters::SourceType(source);
            },
            "Instantiates a SourceType based on provided String value.")
        .def(
            "__repr__",
            [](TripParameters::SourceType type)
            { return osrm_nb_util::enum_to_str(type, "TripSourceType", source_map); },
            "Return a String based on SourceType value.");
    nb::implicitly_convertible<std::string, TripParameters::SourceType>();

    nb::class_<TripParameters::DestinationType>(m, "TripDestinationType")
        .def(
            "__init__",
            [](TripParameters::DestinationType *t, const std::string &str)
            {
                TripParameters::DestinationType dest =
                    osrm_nb_util::str_to_enum(str, "TripDestinationType", dest_map);
                new (t) TripParameters::DestinationType(dest);
            },
            "Instantiates a DestinationType based on provided String value.")
        .def(
            "__repr__",
            [](TripParameters::DestinationType type)
            { return osrm_nb_util::enum_to_str(type, "TripDestinationType", dest_map); },
            "Return a String based on DestinationType value.");
    nb::implicitly_convertible<std::string, TripParameters::DestinationType>();
}
