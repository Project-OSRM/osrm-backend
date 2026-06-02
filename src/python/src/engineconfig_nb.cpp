#include "python/engineconfig_nb.hpp"
#include "python/utility/osrm_utility.hpp"
#include "python/utility/param_utility.hpp"
#include "engine/engine_config.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include <filesystem>

NB_MAKE_OPAQUE(osrm::engine::EngineConfig::Algorithm)

namespace nb = nanobind;

void init_EngineConfig(nb::module_ &m)
{
    using osrm::engine::EngineConfig;

    nb::class_<EngineConfig>(m, "EngineConfig", nb::is_final())
        .def(nb::init<>())
        .def("__init__",
             [](EngineConfig *t, const nb::kwargs &kwargs)
             {
                 new (t) EngineConfig();

                 osrm_nb_util::populate_cfg_from_kwargs(kwargs, *t);

                 if (!t->IsValid())
                 {
                     throw std::runtime_error("Config Parameters are Invalid");
                 }
             })
        .def("IsValid", &EngineConfig::IsValid)
        .def("SetStorageConfig",
             [](EngineConfig &self, const std::string &path)
             { self.storage_config = osrm::storage::StorageConfig(path); })
        .def_rw("max_locations_trip", &EngineConfig::max_locations_trip)
        .def_rw("max_locations_viaroute", &EngineConfig::max_locations_viaroute)
        .def_rw("max_locations_distance_table", &EngineConfig::max_locations_distance_table)
        .def_rw("max_locations_map_matching", &EngineConfig::max_locations_map_matching)
        .def_rw("max_radius_map_matching", &EngineConfig::max_radius_map_matching)
        .def_rw("max_results_nearest", &EngineConfig::max_results_nearest)
        .def_rw("default_radius", &EngineConfig::default_radius)
        .def_rw("max_alternatives", &EngineConfig::max_alternatives)
        .def_rw("use_shared_memory", &EngineConfig::use_shared_memory)
        .def_prop_rw(
            "memory_file",
            [](const EngineConfig &c) { return c.memory_file.string(); },
            [](EngineConfig &c, const std::string &val)
            { c.memory_file = std::filesystem::path(val); })
        .def_rw("use_mmap", &EngineConfig::use_mmap)
        .def_rw("algorithm", &EngineConfig::algorithm)
        .def_rw("verbosity", &EngineConfig::verbosity)
        .def_rw("dataset_name", &EngineConfig::dataset_name);

    nb::class_<EngineConfig::Algorithm>(m, "Algorithm")
        .def("__init__",
             [](EngineConfig::Algorithm *t, const std::string &str)
             {
                 EngineConfig::Algorithm algorithm =
                     osrm_nb_util::str_to_enum(str, "Algorithm", algorithm_map);
                 new (t) EngineConfig::Algorithm(algorithm);
             })
        .def("__repr__",
             [](EngineConfig::Algorithm type)
             { return osrm_nb_util::enum_to_str(type, "Algorithm", algorithm_map); });
    nb::implicitly_convertible<std::string, EngineConfig::Algorithm>();
}
