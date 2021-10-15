
#include <osmium/area/assembler_legacy.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/area/problem_reporter_ogr.hpp>
#include <osmium/geom/ogr.hpp>
#include <osmium/geom/wkt.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/visitor.hpp>

#include <gdalcpp.hpp>

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

struct less_charptr {

    bool operator()(const char* a, const char* b) const noexcept {
        return std::strcmp(a, b) < 0;
    }

}; // less_charptr

using tagmap_type = std::map<const char*, const char*, less_charptr>;

tagmap_type create_map(const osmium::TagList& taglist) {
    tagmap_type map;

    for (auto& tag : taglist) {
        map[tag.key()] = tag.value();
    }

    return map;
}

class TestHandler : public osmium::handler::Handler {

    gdalcpp::Layer m_layer_point;
    gdalcpp::Layer m_layer_lines;
    gdalcpp::Layer m_layer_mpoly;

    osmium::geom::OGRFactory<> m_ogr_factory;
    osmium::geom::WKTFactory<> m_wkt_factory;

    std::ofstream m_out;

    bool m_first_out{true};

public:

    explicit TestHandler(gdalcpp::Dataset& dataset) :
        m_layer_point(dataset, "points", wkbPoint),
        m_layer_lines(dataset, "lines", wkbLineString),
        m_layer_mpoly(dataset, "multipolygons", wkbMultiPolygon),
        m_out("multipolygon-tests.json") {

        m_layer_point.add_field("id", OFTReal, 10);
        m_layer_point.add_field("type", OFTString, 30);

        m_layer_lines.add_field("id", OFTReal, 10);
        m_layer_lines.add_field("type", OFTString, 30);

        m_layer_mpoly.add_field("id", OFTReal, 10);
        m_layer_mpoly.add_field("from_type", OFTString, 1);
    }

    TestHandler(const TestHandler&) = delete;
    TestHandler& operator=(const TestHandler&) = delete;

    TestHandler(TestHandler&&) = delete;
    TestHandler& operator=(TestHandler&&) = delete;

    ~TestHandler() {
        m_out << "\n]\n";
    }

    void node(const osmium::Node& node) {
        gdalcpp::Feature feature{m_layer_point, m_ogr_factory.create_point(node)};
        feature.set_field("id", static_cast<double>(node.id()));
        feature.set_field("type", node.tags().get_value_by_key("type"));
        feature.add_to_layer();
    }

    void way(const osmium::Way& way) {
        try {
            gdalcpp::Feature feature{m_layer_lines, m_ogr_factory.create_linestring(way)};
            feature.set_field("id", static_cast<double>(way.id()));
            feature.set_field("type", way.tags().get_value_by_key("type"));
            feature.add_to_layer();
        } catch (const osmium::geometry_error&) {
            std::cerr << "Ignoring illegal geometry for way " << way.id() << ".\n";
        }
    }

    void area(const osmium::Area& area) {
        if (m_first_out) {
            m_out << "[\n";
            m_first_out = false;
        } else {
            m_out << ",\n";
        }
        m_out << "{\n  \"test_id\": " << (area.orig_id() / 1000) << ",\n  \"area_id\": " << area.id() << ",\n  \"from_id\": " << area.orig_id() << ",\n  \"from_type\": \"" << (area.from_way() ? "way" : "relation") << "\",\n  \"wkt\": \"";
        try {
            const std::string wkt = m_wkt_factory.create_multipolygon(area);
            m_out << wkt << "\",\n  \"tags\": {";

            const auto tagmap = create_map(area.tags());
            bool first = true;
            for (auto& tag : tagmap) {
                if (first) {
                    first = false;
                } else {
                    m_out << ", ";
                }
                m_out << '"' << tag.first << "\": \"" << tag.second << '"';
            }
            m_out << "}\n}";
        } catch (const osmium::geometry_error&) {
            m_out << "INVALID\"\n}";
        }
        try {
            gdalcpp::Feature feature{m_layer_mpoly, m_ogr_factory.create_multipolygon(area)};
            feature.set_field("id", static_cast<double>(area.orig_id()));

            std::string from_type;
            if (area.from_way()) {
                from_type = "w";
            } else {
                from_type = "r";
            }
            feature.set_field("from_type", from_type.c_str());
            feature.add_to_layer();
        } catch (const osmium::geometry_error&) {
            std::cerr << "Ignoring illegal geometry for area " << area.id() << " created from " << (area.from_way() ? "way" : "relation") << " with id=" << area.orig_id() << ".\n";
        }
    }

}; // class TestHandler

/* ================================================== */

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " INFILE\n";
        std::exit(1);
    }

    try {
        const std::string output_format{"SQLite"};
        const std::string input_filename{argv[1]};
        const std::string output_filename{"multipolygon.db"};

        CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "FALSE");
        gdalcpp::Dataset dataset{output_format, output_filename, gdalcpp::SRS{}, {"SPATIALITE=TRUE"}};

        osmium::area::ProblemReporterOGR problem_reporter{dataset};
        osmium::area::AssemblerLegacy::config_type assembler_config;
        assembler_config.problem_reporter = &problem_reporter;
        assembler_config.check_roles = true;
        assembler_config.create_empty_areas = true;
        assembler_config.debug_level = 2;
        osmium::area::MultipolygonCollector<osmium::area::AssemblerLegacy> collector{assembler_config};

        std::cerr << "Pass 1...\n";
        osmium::io::Reader reader1{input_filename};
        collector.read_relations(reader1);
        reader1.close();
        std::cerr << "Pass 1 done\n";

        index_type index;
        location_handler_type location_handler{index};
        location_handler.ignore_errors();

        TestHandler test_handler{dataset};

        std::cerr << "Pass 2...\n";
        osmium::io::Reader reader2{input_filename};
        osmium::apply(reader2, location_handler, test_handler, collector.handler([&test_handler](const osmium::memory::Buffer& area_buffer) {
            osmium::apply(area_buffer, test_handler);
        }));
        reader2.close();
        std::cerr << "Pass 2 done\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        std::exit(1);
    }
}

