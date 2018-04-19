/*****************************************************************************

  Example program for vtzero library.

  vtzero-streets - Copy features from road_label layer if they have property
                   class="street". Output is always in file "streets.mvt".

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/builder.hpp>
#include <vtzero/property_mapper.hpp>
#include <vtzero/vector_tile.hpp>

#include <iostream>
#include <string>

static bool keep_feature(const vtzero::feature& feature) {
    static const std::string key{"class"};
    static const std::string val{"street"};

    bool found = false;

    feature.for_each_property([&](const vtzero::property& prop) {
        found = key == prop.key() && val == prop.value().string_value();
        return !found;
    });

    return found;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TILE\n";
        return 1;
    }

    std::string input_file{argv[1]};
    std::string output_file{"streets.mvt"};

    const auto data = read_file(input_file);

    try {
        vtzero::vector_tile tile{data};

        auto layer = get_layer(tile, "road_label");
        if (!layer) {
            std::cerr << "No 'road_label' layer found\n";
            return 1;
        }

        vtzero::tile_builder tb;
        vtzero::layer_builder layer_builder{tb, layer};

        vtzero::property_mapper mapper{layer, layer_builder};

        while (auto feature = layer.next_feature()) {
            if (keep_feature(feature)) {
                vtzero::geometry_feature_builder feature_builder{layer_builder};
                if (feature.has_id()) {
                    feature_builder.set_id(feature.id());
                }
                feature_builder.set_geometry(feature.geometry());

                while (auto idxs = feature.next_property_indexes()) {
                    feature_builder.add_property(mapper(idxs));
                }
            }
        }

        std::string output = tb.serialize();
        write_data_to_file(output, output_file);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}

