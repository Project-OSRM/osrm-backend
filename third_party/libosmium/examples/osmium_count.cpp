/*

  EXAMPLE osmium_count

  Counts the number of nodes, ways, and relations in the input file.

  DEMONSTRATES USE OF:
  * OSM file input
  * your own handler
  * the memory usage utility class

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdint>  // for std::uint64_t
#include <iostream> // for std::cout, std::cerr

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// We want to use the handler interface
#include <osmium/handler.hpp>

// Utility class gives us access to memory usage information
#include <osmium/util/memory.hpp>

// For osmium::apply()
#include <osmium/visitor.hpp>

// Handler derive from the osmium::handler::Handler base class. Usually you
// overwrite functions node(), way(), and relation(). Other functions are
// available, too. Read the API documentation for details.
struct CountHandler : public osmium::handler::Handler {

    std::uint64_t nodes     = 0;
    std::uint64_t ways      = 0;
    std::uint64_t relations = 0;

    // This callback is called by osmium::apply for each node in the data.
    void node(const osmium::Node& /*node*/) noexcept {
        ++nodes;
    }

    // This callback is called by osmium::apply for each way in the data.
    void way(const osmium::Way& /*way*/) noexcept {
        ++ways;
    }

    // This callback is called by osmium::apply for each relation in the data.
    void relation(const osmium::Relation& /*relation*/) noexcept {
        ++relations;
    }

}; // struct CountHandler


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        return 1;
    }

    try {
        // The Reader is initialized here with an osmium::io::File, but could
        // also be directly initialized with a file name.
        const osmium::io::File input_file{argv[1]};
        osmium::io::Reader reader{input_file};

        // Create an instance of our own CountHandler and push the data from the
        // input file through it.
        CountHandler handler;
        osmium::apply(reader, handler);

        // You do not have to close the Reader explicitly, but because the
        // destructor can't throw, you will not see any errors otherwise.
        reader.close();

        std::cout << "Nodes: "     << handler.nodes << "\n";
        std::cout << "Ways: "      << handler.ways << "\n";
        std::cout << "Relations: " << handler.relations << "\n";

        // Because of the huge amount of OSM data, some Osmium-based programs
        // (though not this one) can use huge amounts of data. So checking actual
        // memore usage is often useful and can be done easily with this class.
        // (Currently only works on Linux, not macOS and Windows.)
        const osmium::MemoryUsage memory;

        std::cout << "\nMemory used: " << memory.peak() << " MBytes\n";
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        return 1;
    }
}

