/*****************************************************************************

  Utility functions for vtzero example programs.

*****************************************************************************/

#include "utils.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

/**
 * Read complete contents of a file into a string.
 *
 * The file is read in binary mode.
 *
 * @param filename The file name. Can be empty or "-" to read from STDIN.
 * @returns a string with the contents of the file.
 * @throws various exceptions if there is an error
 */
std::string read_file(const std::string& filename) {
    if (filename.empty() || (filename.size() == 1 && filename[0] == '-')) {
        return std::string{std::istreambuf_iterator<char>(std::cin.rdbuf()),
                           std::istreambuf_iterator<char>()};
    }

    std::ifstream stream{filename, std::ios_base::in | std::ios_base::binary};
    if (!stream) {
        throw std::runtime_error{std::string{"Can not open file '"} + filename + "'"};
    }

    stream.exceptions(std::ifstream::failbit);

    std::string buffer{std::istreambuf_iterator<char>(stream.rdbuf()),
                       std::istreambuf_iterator<char>()};
    stream.close();

    return buffer;
}

/**
 * Write contents of a buffer into a file.
 *
 * The file is written in binary mode.
 *
 * @param buffer The data to be written.
 * @param filename The file name.
 * @throws various exceptions if there is an error
 */
void write_data_to_file(const std::string& buffer, const std::string& filename) {
    std::ofstream stream{filename, std::ios_base::out | std::ios_base::binary};
    if (!stream) {
        throw std::runtime_error{std::string{"Can not open file '"} + filename + "'"};
    }

    stream.exceptions(std::ifstream::failbit);

    stream.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    stream.close();
}

/**
 * Get a specific layer from a vector tile. The layer can be specified as a
 * number n in which case the nth layer in this tile is returned. Or it can
 * be specified as text, in which case the layer with that name is returned.
 *
 * Calls exit(1) if there is an error.
 *
 * @param tile The vector tile.
 * @param layer_name_or_num specifies the layer.
 */
vtzero::layer get_layer(const vtzero::vector_tile& tile, const std::string& layer_name_or_num) {
    vtzero::layer layer;
    char* str_end = nullptr;
    const long num = std::strtol(layer_name_or_num.c_str(), &str_end, 10); // NOLINT(google-runtime-int)

    if (str_end == layer_name_or_num.data() + layer_name_or_num.size()) {
        if (num >= 0 && num < std::numeric_limits<long>::max()) { // NOLINT(google-runtime-int)
            layer = tile.get_layer(static_cast<std::size_t>(num));
            if (!layer) {
                std::cerr << "No such layer: " << num << '\n';
                std::exit(1);
            }
            return layer;
        }
    }

    layer = tile.get_layer_by_name(layer_name_or_num);
    if (!layer) {
        std::cerr << "No layer named '" << layer_name_or_num << "'.\n";
        std::exit(1);
    }
    return layer;
}

