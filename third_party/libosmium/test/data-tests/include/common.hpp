#ifndef COMMON_HPP
#define COMMON_HPP

#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>

#include <osmium/geom/wkt.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/visitor.hpp>

using index_neg_type = osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location>;
using index_pos_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type>;

#include "check_basics_handler.hpp"
#include "check_wkt_handler.hpp"

#include "testdata-testcases.hpp"

#endif // COMMON_HPP
