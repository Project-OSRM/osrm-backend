/*

  Read OSM changesets with discussions from a changeset dump like the one
  you get from http://planet.osm.org/planet/discussions-latest.osm.bz2
  and write out only those changesets which have discussions (ie comments).

  The code in this example file is released into the Public Domain.

*/

#include <algorithm> // for std::copy_if
#include <iostream> // for std::cout, std::cerr

// we want to read OSM files in XML format
// (other formats don't support full changesets, so only XML is needed here)
#include <osmium/io/xml_input.hpp>
#include <osmium/io/input_iterator.hpp>

// we want to write OSM files in XML format
#include <osmium/io/xml_output.hpp>
#include <osmium/io/output_iterator.hpp>

// we want to support any compressioon (.gz2 and .bz2)
#include <osmium/io/any_compression.hpp>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " INFILE OUTFILE\n";
        exit(1);
    }

    // The input file, deduce file format from file suffix
    osmium::io::File infile(argv[1]);

    // The output file, force class XML OSM file format
    osmium::io::File outfile(argv[2], "osm");

    // Initialize Reader for the input file.
    // Read only changesets (will ignore nodes, ways, and
    // relations if there are any).
    osmium::io::Reader reader(infile, osmium::osm_entity_bits::changeset);

    // Get the header from the input file
    osmium::io::Header header = reader.header();

    // Initialize writer for the output file. Use the header from the input
    // file for the output file. This will copy over some header information.
    // The last parameter will tell the writer that it is allowed to overwrite
    // an existing file. Without it, it will refuse to do so.
    osmium::io::Writer writer(outfile, header, osmium::io::overwrite::allow);

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

