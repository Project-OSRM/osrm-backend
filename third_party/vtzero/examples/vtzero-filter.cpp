/*****************************************************************************

  Example program for vtzero library.

  vtzero-filter - Copy parts of a vector tile into a new tile.

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/builder.hpp>
#include <vtzero/vector_tile.hpp>

#include <clara.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>

int main(int argc, char* argv[]) {
    std::string filename;
    std::string layer_num_or_name;
    std::string idstr;
    std::string output_file{"filtered.mvt"};

    bool help = false;

    const auto cli
        = clara::Opt(output_file, "FILE")
            ["-o"]["--output"]
            ("write output to FILE")
        | clara::Help(help)
        | clara::Arg(filename, "FILENAME").required()
            ("vector tile")
        | clara::Arg(layer_num_or_name, "LAYER-NUM|LAYER-NAME").required()
            ("layer")
        | clara::Arg(idstr, "ID")
            ("feature_id");

    const auto result = cli.parse(clara::Args(argc, argv));
    if (!result) {
        std::cerr << "Error in command line: " << result.errorMessage() << '\n';
        return 1;
    }

    if (help) {
        std::cout << cli
                  << "\nFilter contents of vector tile.\n";
        return 0;
    }

    if (filename.empty()) {
        std::cerr << "Error in command line: Missing file name of vector tile to read\n";
        return 1;
    }

    if (layer_num_or_name.empty()) {
        std::cerr << "Error in command line: Missing layer number or name\n";
        return 1;
    }

    const auto data = read_file(filename);
    vtzero::vector_tile tile{data};

    auto layer = get_layer(tile, layer_num_or_name);
    std::cerr << "Found layer: " << std::string(layer.name()) << "\n";

    vtzero::tile_builder tb;

    if (idstr.empty()) {
        tb.add_existing_layer(layer);
    } else {
        char* str_end = nullptr;
        const int64_t id = std::strtoll(idstr.c_str(), &str_end, 10);
        if (str_end != idstr.c_str() + idstr.size()) {
            std::cerr << "Feature ID must be numeric.\n";
            return 1;
        }
        if (id < 0) {
            std::cerr << "Feature ID must be >= 0.\n";
            return 1;
        }

        const auto feature = layer.get_feature_by_id(static_cast<uint64_t>(id));
        if (!feature.valid()) {
            std::cerr << "No feature with that id: " << id << '\n';
            return 1;
        }

        vtzero::layer_builder layer_builder{tb, layer};
        layer_builder.add_feature(feature);
    }

    std::string output = tb.serialize();

    write_data_to_file(output, output_file);
}

