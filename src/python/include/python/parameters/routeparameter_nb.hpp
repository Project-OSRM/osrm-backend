#ifndef OSRM_NB_ROUTEPARAMETER_H
#define OSRM_NB_ROUTEPARAMETER_H

#include "python/parameters/baseparameter_nb.hpp"
#include "engine/api/route_parameters.hpp"

#include <nanobind/nanobind.h>

#include <unordered_map>

using osrm::engine::api::RouteParameters;

// Must be visible in every TU that converts these enum types to/from Python.
NB_MAKE_OPAQUE(osrm::engine::api::RouteParameters::GeometriesType)
NB_MAKE_OPAQUE(osrm::engine::api::RouteParameters::OverviewType)
NB_MAKE_OPAQUE(osrm::engine::api::RouteParameters::AnnotationsType)

void init_RouteParameters(nanobind::module_ &m);

static const std::unordered_map<std::string, RouteParameters::GeometriesType> geometries_map{
    {"polyline", RouteParameters::GeometriesType::Polyline},
    {std::string(), RouteParameters::GeometriesType::Polyline},
    {"polyline6", RouteParameters::GeometriesType::Polyline6},
    {"geojson", RouteParameters::GeometriesType::GeoJSON}};

static const std::unordered_map<std::string, RouteParameters::OverviewType> overview_map{
    {"simplified", RouteParameters::OverviewType::Simplified},
    {std::string(), RouteParameters::OverviewType::Simplified},
    {"full", RouteParameters::OverviewType::Full},
    {"false", RouteParameters::OverviewType::False}};

static const std::unordered_map<std::string, RouteParameters::AnnotationsType>
    route_annotations_map{{"none", RouteParameters::AnnotationsType::None},
                          {std::string(), RouteParameters::AnnotationsType::None},
                          {"duration", RouteParameters::AnnotationsType::Duration},
                          {"nodes", RouteParameters::AnnotationsType::Nodes},
                          {"distance", RouteParameters::AnnotationsType::Distance},
                          {"weight", RouteParameters::AnnotationsType::Weight},
                          {"datasources", RouteParameters::AnnotationsType::Datasources},
                          {"speed", RouteParameters::AnnotationsType::Speed},
                          {"all", RouteParameters::AnnotationsType::All}};

#endif // OSRM_NB_ROUTEPARAMETER_H
