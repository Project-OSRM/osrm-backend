/*

  The code in this file is released into the Public Domain.

*/

#include <osmium/geom/mercator_projection.hpp>
#include <osmium/geom/wkb.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

struct GeomHandler : public osmium::handler::Handler {

    osmium::geom::WKBFactory<osmium::geom::MercatorProjection> factory;

    void node(const osmium::Node& node) {
        const std::string geom = factory.create_point(node);
    }

};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        std::exit(1);
    }

    const std::string input_filename{argv[1]};

    osmium::io::Reader reader{input_filename};

    GeomHandler handler;
    osmium::apply(reader, handler);
    reader.close();
}

