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
#include <exception> // for std::exception
#include <getopt.h>  // for getopt_long
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

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help",        no_argument, 0, 'h'},
        {"from-format", required_argument, 0, 'f'},
        {"to-format",   required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    // Input and output format are empty by default. Later this will mean that
    // the format should be taken from the input and output file suffix,
    // respectively.
    std::string input_format;
    std::string output_format;

    // Read options from command line.
    while (true) {
        const int c = getopt_long(argc, argv, "dhf:t:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                std::exit(0);
            case 'f':
                input_format = optarg;
                break;
            case 't':
                output_format = optarg;
                break;
            default:
                std::exit(1);
        }
    }

    const int remaining_args = argc - optind;
    if (remaining_args > 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE [OUTFILE]]\n";
        std::exit(1);
    }

    // Get input file name from command line.
    std::string input_file_name;
    if (remaining_args >= 1) {
        input_file_name = argv[optind];
    }

    // Get output file name from command line.
    std::string output_file_name;
    if (remaining_args == 2) {
        output_file_name = argv[optind+1];
    }

    // This declares the input and output files using either the suffix of
    // the file names or the format in the 2nd argument. It does not yet open
    // the files.
    osmium::io::File input_file{input_file_name, input_format};
    osmium::io::File output_file{output_file_name, output_format};

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
        while (osmium::memory::Buffer buffer = reader.read()) {
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
        std::cerr << e.what() << "\n";
        std::exit(1);
    }
}

