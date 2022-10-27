/*****************************************************************************

  Example program for vtzero library.

  vtzero-show - Show content of vector tile

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/vector_tile.hpp>

#include <clara.hpp>

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

class geom_handler {

    std::string output{};

public:

    void points_begin(const uint32_t /*count*/) const noexcept {
    }

    void points_point(const vtzero::point point) const {
        std::cout << "      POINT(" << point.x << ',' << point.y << ")\n";
    }

    void points_end() const noexcept {
    }

    void linestring_begin(const uint32_t count) {
        output = "      LINESTRING[count=";
        output += std::to_string(count);
        output += "](";
    }

    void linestring_point(const vtzero::point point) {
        output += std::to_string(point.x);
        output += ' ';
        output += std::to_string(point.y);
        output += ',';
    }

    void linestring_end() {
        if (output.empty()) {
            return;
        }
        if (output.back() == ',') {
            output.resize(output.size() - 1);
        }
        output += ")\n";
        std::cout << output;
    }

    void ring_begin(const uint32_t count) {
        output = "      RING[count=";
        output += std::to_string(count);
        output += "](";
    }

    void ring_point(const vtzero::point point) {
        output += std::to_string(point.x);
        output += ' ';
        output += std::to_string(point.y);
        output += ',';
    }

    void ring_end(const vtzero::ring_type rt) {
        if (output.empty()) {
            return;
        }
        if (output.back() == ',') {
            output.back() = ')';
        }
        switch (rt) {
            case vtzero::ring_type::outer:
                output += "[OUTER]\n";
                break;
            case vtzero::ring_type::inner:
                output += "[INNER]\n";
                break;
            default:
                output += "[INVALID]\n";
        }
        std::cout << output;
    }

}; // class geom_handler

template <typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, vtzero::data_view value) {
    out.write(value.data(), static_cast<std::streamsize>(value.size()));
    return out;
}

struct print_value {

    template <typename T>
    void operator()(T value) const {
        std::cout << value;
    }

    void operator()(const vtzero::data_view value) const {
        std::cout << '"' << value << '"';
    }

}; // struct print_value

static void print_layer(vtzero::layer& layer, bool print_tables, bool print_value_types, int& layer_num, int& feature_num) {
    std::cout << "=============================================================\n"
              << "layer: " << layer_num << '\n'
              << "  name: " << std::string(layer.name()) << '\n'
              << "  version: " << layer.version() << '\n'
              << "  extent: " << layer.extent() << '\n';

    if (print_tables) {
        std::cout << "  keys:\n";
        int n = 0;
        for (const auto& key : layer.key_table()) {
            std::cout << "    " << n++ << ": " << key << '\n';
        }
        std::cout << "  values:\n";
        n = 0;
        for (const vtzero::property_value& value : layer.value_table()) {
            std::cout << "    " << n++ << ": ";
            vtzero::apply_visitor(print_value{}, value);
            if (print_value_types) {
                std::cout << " [" << vtzero::property_value_type_name(value.type()) << "]\n";
            } else {
                std::cout << '\n';
            }
        }
    }

    feature_num = 0;
    while (auto feature = layer.next_feature()) {
        std::cout << "  feature: " << feature_num << '\n'
                  << "    id: ";
        if (feature.has_id()) {
            std::cout << feature.id() << '\n';
        } else {
            std::cout << "(none)\n";
        }
        std::cout << "    geomtype: " << vtzero::geom_type_name(feature.geometry_type()) << '\n'
                  << "    geometry:\n";
        vtzero::decode_geometry(feature.geometry(), geom_handler{});
        std::cout << "    properties:\n";
        while (auto property = feature.next_property()) {
            std::cout << "      " << property.key() << '=';
            vtzero::apply_visitor(print_value{}, property.value());
            if (print_value_types) {
                std::cout << " [" << vtzero::property_value_type_name(property.value().type()) << "]\n";
            } else {
                std::cout << '\n';
            }
        }
        ++feature_num;
    }
}

static void print_layer_overview(const vtzero::layer& layer) {
    std::cout << layer.name() << ' ' << layer.num_features() << '\n';
}

int main(int argc, char* argv[]) {
    std::string filename;
    std::string layer_num_or_name;
    bool layer_overview = false;
    bool print_tables = false;
    bool print_value_types = false;
    bool help = false;

    const auto cli
        = clara::Opt(layer_overview)
            ["-l"]["--layers"]
            ("show layer overview with feature count")
        | clara::Opt(print_tables)
            ["-t"]["--tables"]
            ("also print key/value tables")
        | clara::Opt(print_value_types)
            ["-T"]["--value-types"]
            ("also show value types")
        | clara::Help(help)
        | clara::Arg(filename, "FILENAME").required()
            ("vector tile")
        | clara::Arg(layer_num_or_name, "LAYER-NUM|LAYER-NAME")
            ("layer");

    const auto result = cli.parse(clara::Args(argc, argv));
    if (!result) {
        std::cerr << "Error in command line: " << result.errorMessage() << '\n';
        return 1;
    }

    if (help) {
        std::cout << cli
                  << "\nShow contents of vector tile FILENAME.\n";
        return 0;
    }

    if (filename.empty()) {
        std::cerr << "Error in command line: Missing file name of vector tile to read\n";
        return 1;
    }

    int layer_num = 0;
    int feature_num = 0;
    try {
        const auto data = read_file(filename);

        vtzero::vector_tile tile{data};

        if (layer_num_or_name.empty()) {
            while (auto layer = tile.next_layer()) {
                if (layer_overview) {
                    print_layer_overview(layer);
                } else {
                    print_layer(layer, print_tables, print_value_types, layer_num, feature_num);
                }
                ++layer_num;
            }
        } else {
            auto layer = get_layer(tile, layer_num_or_name);
            if (layer_overview) {
                print_layer_overview(layer);
            } else {
                print_layer(layer, print_tables, print_value_types, layer_num, feature_num);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in layer " << layer_num << " (feature " << feature_num << "): " << e.what() << '\n';
        return 1;
    }

    return 0;
}

