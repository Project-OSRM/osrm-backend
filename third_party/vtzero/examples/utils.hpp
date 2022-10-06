
#include <vtzero/vector_tile.hpp>

#include <string>

std::string read_file(const std::string& filename);

void write_data_to_file(const std::string& buffer, const std::string& filename);

vtzero::layer get_layer(const vtzero::vector_tile& tile, const std::string& layer_name_or_num);

