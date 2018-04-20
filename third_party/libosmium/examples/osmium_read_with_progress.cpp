/*

  EXAMPLE osmium_read_with_progress

  Reads the contents of the input file showing a progress bar.

  DEMONSTRATES USE OF:
  * file input
  * ProgressBar utility function

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdlib>  // for std::exit
#include <iostream> // for std::cerr

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// Get access to isatty utility function and progress bar utility class.
#include <osmium/util/file.hpp>
#include <osmium/util/progress_bar.hpp>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        std::exit(1);
    }

    // The Reader is initialized here with an osmium::io::File, but could
    // also be directly initialized with a file name.
    osmium::io::File input_file{argv[1]};
    osmium::io::Reader reader{input_file};

    // Initialize progress bar, enable it only if STDERR is a TTY.
    osmium::ProgressBar progress{reader.file_size(), osmium::isatty(2)};

    // OSM data comes in buffers, read until there are no more.
    while (osmium::memory::Buffer buffer = reader.read()) {
        // Update progress bar for each buffer.
        progress.update(reader.offset());
    }

    // Progress bar is done.
    progress.done();

    // You do not have to close the Reader explicitly, but because the
    // destructor can't throw, you will not see any errors otherwise.
    reader.close();
}

