/*

  EXAMPLE osmium_change_tags

  An example how tags in OSM files can be removed or changed. Removes
  "created_by" tags and changes tag "landuse=forest" into "natural_wood".

  DEMONSTRATES USE OF:
  * file input and output
  * Osmium buffers
  * your own handler
  * access to tags
  * using builders to write data

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count
  * osmium_pub_names

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdlib>   // for std::exit
#include <cstring>   // for std::strcmp
#include <exception> // for std::exception
#include <iostream>  // for std::cout, std::cerr
#include <string>    // for std::string
#include <utility>   // for std::move

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// Allow any format of output files (XML, PBF, ...)
#include <osmium/io/any_output.hpp>

// We want to use the builder interface
#include <osmium/builder/osm_object_builder.hpp>

// We want to use the handler interface
#include <osmium/handler.hpp>

// For osmium::apply()
#include <osmium/visitor.hpp>

// The functions in this class will be called for each object in the input
// and will write a (changed) copy of those objects to the given buffer.
class RewriteHandler : public osmium::handler::Handler {

    osmium::memory::Buffer& m_buffer;

    // Copy attributes common to all OSM objects (nodes, ways, and relations).
    template <typename T>
    void copy_attributes(T& builder, const osmium::OSMObject& object) {
        // The setter functions on the builder object all return the same
        // builder object so they can be chained.
        builder.set_id(object.id())
            .set_version(object.version())
            .set_changeset(object.changeset())
            .set_timestamp(object.timestamp())
            .set_uid(object.uid())
            .set_user(object.user());
    }

    // Copy all tags with two changes:
    // * Do not copy "created_by" tags
    // * Change "landuse=forest" into "natural=wood"
    static void copy_tags(osmium::builder::Builder& parent, const osmium::TagList& tags) {

        // The TagListBuilder is used to create a list of tags. The parameter
        // to create it is a reference to the builder of the object that
        // should have those tags.
        osmium::builder::TagListBuilder builder{parent};

        // Iterate over all tags and build new tags using the new builder
        // based on the old ones.
        for (const auto& tag : tags) {
            if (!std::strcmp(tag.key(), "created_by")) {
                // ignore
            } else if (!std::strcmp(tag.key(), "landuse") && !std::strcmp(tag.value(), "forest")) {
                // add_tag() can be called with key and value C strings
                builder.add_tag("natural", "wood");
            } else {
                // add_tag() can also be called with an osmium::Tag
                builder.add_tag(tag);
            }
        }
    }

public:

    // Constructor. New data will be added to the given buffer.
    explicit RewriteHandler(osmium::memory::Buffer& buffer) :
        m_buffer(buffer) {
    }

    // The node handler is called for each node in the input data.
    void node(const osmium::Node& node) {
        // Open a new scope, because the NodeBuilder we are creating has to
        // be destructed, before we can call commit() below.
        {
            // To create a node, we need a NodeBuilder object. It will create
            // the node in the given buffer.
            osmium::builder::NodeBuilder builder{m_buffer};

            // Copy common object attributes over to the new node.
            copy_attributes(builder, node);

            // Copy the location over to the new node.
            builder.set_location(node.location());

            // Copy (changed) tags.
            copy_tags(builder, node.tags());
        }

        // Once the object is written to the buffer completely, we have to call
        // commit().
        m_buffer.commit();
    }

    // The way handler is called for each way in the input data.
    void way(const osmium::Way& way) {
        {
            osmium::builder::WayBuilder builder{m_buffer};
            copy_attributes(builder, way);
            copy_tags(builder, way.tags());

            // Copy the node list over to the new way.
            builder.add_item(way.nodes());
        }
        m_buffer.commit();
    }

    // The relation handler is called for each relation in the input data.
    void relation(const osmium::Relation& relation) {
        {
            osmium::builder::RelationBuilder builder{m_buffer};
            copy_attributes(builder, relation);
            copy_tags(builder, relation.tags());

            // Copy the relation member list over to the new way.
            builder.add_item(relation.members());
        }
        m_buffer.commit();
    }

}; // class RewriteHandler

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " INFILE OUTFILE\n";
        std::exit(1);
    }

    // Get input and output file names from command line.
    std::string input_file_name{argv[1]};
    std::string output_file_name{argv[2]};

    try {
        // Initialize Reader
        osmium::io::Reader reader{input_file_name};

        // Get header from input file and change the "generator" setting to
        // ourselves.
        osmium::io::Header header = reader.header();
        header.set("generator", "osmium_change_tags");

        // Initialize Writer using the header from above and tell it that it
        // is allowed to overwrite a possibly existing file.
        osmium::io::Writer writer{output_file_name, header, osmium::io::overwrite::allow};

        // Read in buffers with OSM objects until there are no more.
        while (osmium::memory::Buffer input_buffer = reader.read()) {
            // Create an empty buffer with the same size as the input buffer.
            // We'll copy the changed data into output buffer, the changes
            // are small, so the output buffer needs to be about the same size.
            // In case it has to be bigger, we allow it to grow automatically
            // by adding the auto_grow::yes parameter.
            osmium::memory::Buffer output_buffer{input_buffer.committed(), osmium::memory::Buffer::auto_grow::yes};

            // Construct a handler as defined above and feed the input buffer
            // to it.
            RewriteHandler handler{output_buffer};
            osmium::apply(input_buffer, handler);

            // Write out the contents of the output buffer.
            writer(std::move(output_buffer));
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

