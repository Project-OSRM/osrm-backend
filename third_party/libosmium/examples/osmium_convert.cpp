/*

  EXAMPLE osmium_convert

  Convert OSM files from one format into another.

  DEMONSTRATES USE OF:
  * file input and output
  * file types
  * Osmium buffers

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdlib>   // for std::exit
#include <cstring>   // for std::strcmp
#include <exception> // for std::exception
#include <iostream>  // for std::cout, std::cerr
#include <string>    // for std::string

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// Allow any format of output files (XML, PBF, ...)
#include <osmium/io/any_output.hpp>

void print_help() {
    std::cout << "osmium_convert [OPTIONS] [INFILE [OUTFILE]]\n\n" \
              << "If INFILE or OUTFILE is not given stdin/stdout is assumed.\n" \
              << "File format is autodetected from file name suffix.\n" \
              << "Use -f and -t options to force file format.\n" \
              << "\nFile types:\n" \
              << "  osm        normal OSM file\n" \
              << "  osc        OSM change file\n" \
              << "  osh        OSM file with history information\n" \
              << "\nFile format:\n" \
              << "  (default)  XML encoding\n" \
              << "  pbf        binary PBF encoding\n" \
              << "  opl        OPL encoding\n" \
              << "\nFile compression\n" \
              << "  gz         compressed with gzip\n" \
              << "  bz2        compressed with bzip2\n" \
              << "\nOptions:\n" \
              << "  -h, --help                This help message\n" \
              << "  -f, --from-format=FORMAT  Input format\n" \
              << "  -t, --to-format=FORMAT    Output format\n";
}

void print_usage(const char* prgname) {
    std::cerr << "Usage: " << prgname << " [OPTIONS] [INFILE [OUTFILE]]\n";
    std::exit(0);
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        print_usage(argv[0]);
    }

    if (argc > 1 && (!std::strcmp(argv[1], "-h") ||
                     !std::strcmp(argv[1], "--help"))) {
        print_help();
        std::exit(0);
    }

    // Input and output format are empty by default. Later this will mean that
    // the format should be taken from the input and output file suffix,
    // respectively.
    std::string input_format;
    std::string output_format;

    std::string input_file_name;
    std::string output_file_name;

    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "-f") || !std::strcmp(argv[i], "--from-format")) {
            ++i;
            if (i < argc) {
                input_format = argv[i];
            } else {
                print_usage(argv[0]);
            }
        } else if (!std::strncmp(argv[i], "--from-format=", 14)) {
            input_format = argv[i] + 14;
        } else if (!std::strcmp(argv[i], "-t") || !std::strcmp(argv[i], "--to-format")) {
            ++i;
            if (i < argc) {
                output_format = argv[i];
            } else {
                print_usage(argv[0]);
            }
        } else if (!std::strncmp(argv[i], "--to-format=", 12)) {
            output_format = argv[i] + 12;
        } else if (input_file_name.empty()) {
            input_file_name = argv[i];
        } else if (output_file_name.empty()) {
            output_file_name = argv[i];
        } else {
            print_usage(argv[0]);
        }
    }

    // This declares the input and output files using either the suffix of
    // the file names or the format in the 2nd argument. It does not yet open
    // the files.
    const osmium::io::File input_file{input_file_name, input_format};
    const osmium::io::File output_file{output_file_name, output_format};

    // Input and output files can be OSM data files (without history) or
    // OSM history files. History files are detected if they use the '.osh'
    // file suffix.
    if (  input_file.has_multiple_object_versions() &&
        !output_file.has_multiple_object_versions()) {
        std::cerr << "Warning! You are converting from an OSM file with (potentially) several versions of the same object to one that is not marked as such.\n";
    }

    try {
        // Initialize Reader
        osmium::io::Reader reader{input_file};

        // Get header from input file and change the "generator" setting to
        // ourselves.
        osmium::io::Header header = reader.header();
        header.set("generator", "osmium_convert");

        // Initialize Writer using the header from above and tell it that it
        // is allowed to overwrite a possibly existing file.
        osmium::io::Writer writer{output_file, header, osmium::io::overwrite::allow};

        // Copy the contents from the input to the output file one buffer at
        // a time. This is much easier and faster than copying each object
        // in the file. Buffers are moved around, so there is no cost for
        // copying in memory.
        while (osmium::memory::Buffer buffer = reader.read()) { // NOLINT(bugprone-use-after-move) Bug in clang-tidy https://bugs.llvm.org/show_bug.cgi?id=36516
            writer(std::move(buffer));
        }

        // Explicitly close the writer and reader. Will throw an exception if
        // there is a problem. If you wait for the destructor to close the writer
        // and reader, you will not notice the problem, because destructors must
        // not throw.
        writer.close();
        reader.close();
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        std::exit(1);
    }
}

