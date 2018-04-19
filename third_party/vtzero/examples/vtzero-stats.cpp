/*****************************************************************************

  Example program for vtzero library.

  vtzero-stats - Output some stats on layers

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/vector_tile.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TILE\n";
        return 1;
    }

    std::string input_file{argv[1]};
    const auto data = read_file(input_file);

    vtzero::vector_tile tile{data};

    while (const auto layer = tile.next_layer()) {
        std::cout.write(layer.name().data(), static_cast<std::streamsize>(layer.name().size()));
        std::cout << ' '
                  << layer.num_features() << ' '
                  << layer.key_table().size() << ' '
                  << layer.value_table().size() << '\n';
    }
}

