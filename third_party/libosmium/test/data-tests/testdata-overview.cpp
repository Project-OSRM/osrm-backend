/* The code in this file is released into the Public Domain. */

#include <osmium/geom/ogr.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/visitor.hpp>

#include <gdalcpp.hpp>

#include <iostream>
#include <string>

using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

class TestOverviewHandler : public osmium::handler::Handler {

    gdalcpp::Layer m_layer_nodes;
    gdalcpp::Layer m_layer_labels;
    gdalcpp::Layer m_layer_ways;

    osmium::geom::OGRFactory<> m_factory;

public:

    explicit TestOverviewHandler(gdalcpp::Dataset& dataset) :
        m_layer_nodes(dataset, "nodes", wkbPoint),
        m_layer_labels(dataset, "labels", wkbPoint),
        m_layer_ways(dataset, "ways", wkbLineString) {

        m_layer_nodes.add_field("id", OFTReal, 10);

        m_layer_labels.add_field("id", OFTReal, 10);
        m_layer_labels.add_field("label", OFTString, 30);

        m_layer_ways.add_field("id", OFTReal, 10);
        m_layer_ways.add_field("test", OFTInteger, 3);
    }

    void node(const osmium::Node& node) {
        const char* label = node.tags().get_value_by_key("label");
        if (label) {
            gdalcpp::Feature feature(m_layer_labels, m_factory.create_point(node));
            feature.set_field("id", static_cast<double>(node.id()));
            feature.set_field("label", label);
            feature.add_to_layer();
        } else {
            gdalcpp::Feature feature(m_layer_nodes, m_factory.create_point(node));
            feature.set_field("id", static_cast<double>(node.id()));
            feature.add_to_layer();
        }
    }

    void way(const osmium::Way& way) {
        try {
            gdalcpp::Feature feature(m_layer_ways, m_factory.create_linestring(way));
            feature.set_field("id", static_cast<double>(way.id()));

            const char* test = way.tags().get_value_by_key("test");
            if (test) {
                feature.set_field("test", test);
            }

            feature.add_to_layer();
        } catch (const osmium::geometry_error&) {
            std::cerr << "Ignoring illegal geometry for way " << way.id() << ".\n";
        }
    }

};

/* ================================================== */

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " INFILE\n";
        return 1;
    }

    try {
        const std::string output_format{"SQLite"};
        const std::string input_filename{argv[1]};
        const std::string output_filename{"testdata-overview.db"};
        ::unlink(output_filename.c_str());

        CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "FALSE");
        gdalcpp::Dataset dataset{output_format, output_filename, gdalcpp::SRS{}, {"SPATIALITE=TRUE"}};

        osmium::io::Reader reader{input_filename};

        index_type index;
        location_handler_type location_handler{index};
        location_handler.ignore_errors();

        TestOverviewHandler handler{dataset};

        osmium::apply(reader, location_handler, handler);
        reader.close();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}

