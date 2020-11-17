/*

  EXAMPLE osmium_read

  Reads and discards the contents of the input file.
  (It can be used for timing.)

  DEMONSTRATES USE OF:
  * file input

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdlib>  // for std::exit
#include <iostream> // for std::cerr

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        std::exit(1);
    }

    try {
        // The Reader is initialized here with an osmium::io::File, but could
        // also be directly initialized with a file name.
        osmium::io::File input_file{argv[1]};
        osmium::io::Reader reader{input_file};

        // OSM data comes in buffers, read until there are no more.
        while (osmium::memory::Buffer buffer = reader.read()) {
            // do nothing
        }

        // You do not have to close the Reader explicitly, but because the
        // destructor can't throw, you will not see any errors otherwise.
        reader.close();
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        std::exit(1);
    }
}

