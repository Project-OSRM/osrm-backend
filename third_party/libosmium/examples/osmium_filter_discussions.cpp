/*

  EXAMPLE osmium_filter_discussions

  Read OSM changesets with discussions from a changeset dump like the one
  you get from https://planet.osm.org/planet/discussions-latest.osm.bz2
  and write out only those changesets which have discussions (ie comments).

  DEMONSTRATES USE OF:
  * file input and output
  * setting file formats using the osmium::io::File class
  * OSM file headers
  * input and output iterators

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <algorithm> // for std::copy_if
#include <cstdlib>   // for std::exit
#include <iostream>  // for std::cout, std::cerr

// We want to read OSM files in XML format
// (other formats don't support full changesets, so only XML is needed here).
#include <osmium/io/xml_input.hpp>

// We want to write OSM files in XML format.
#include <osmium/io/xml_output.hpp>

// We want to use input and output iterators for easy integration with the
// algorithms of the standard library.
#include <osmium/io/input_iterator.hpp>
#include <osmium/io/output_iterator.hpp>

// We want to support any compression (none, gzip, and bzip2).
#include <osmium/io/any_compression.hpp>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " INFILE OUTFILE\n";
        std::exit(1);
    }

    // The input file, deduce file format from file suffix.
    osmium::io::File input_file{argv[1]};

    // The output file, force XML OSM file format.
    osmium::io::File output_file{argv[2], "osm"};

    // Initialize Reader for the input file.
    // Read only changesets (will ignore nodes, ways, and
    // relations if there are any).
    osmium::io::Reader reader{input_file, osmium::osm_entity_bits::changeset};

    // Get the header from the input file.
    osmium::io::Header header = reader.header();

    // Set the "generator" on the header to ourselves.
    header.set("generator", "osmium_filter_discussions");

    // Initialize writer for the output file. Use the header from the input
    // file for the output file. This will copy over some header information.
    // The last parameter will tell the writer that it is allowed to overwrite
    // an existing file. Without it, it will refuse to do so.
    osmium::io::Writer writer{output_file, header, osmium::io::overwrite::allow};

    // Create range of input iterators that will iterator over all changesets
    // delivered from input file through the "reader".
    auto input_range = osmium::io::make_input_iterator_range<osmium::Changeset>(reader);

    // Create an output iterator writing through the "writer" object to the
    // output file.
    auto output_iterator = osmium::io::make_output_iterator(writer);

    // Copy all changesets from input to output that have at least one comment.
    std::copy_if(input_range.begin(), input_range.end(), output_iterator, [](const osmium::Changeset& changeset) {
        return changeset.num_comments() > 0;
    });

    // Explicitly close the writer and reader. Will throw an exception if
    // there is a problem. If you wait for the destructor to close the writer
    // and reader, you will not notice the problem, because destructors must
    // not throw.
    writer.close();
    reader.close();
}

