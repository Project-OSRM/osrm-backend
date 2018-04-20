/*

  EXAMPLE osmium_dump_internal

  Reads an OSM file and dumps the internal datastructure to disk including
  indexes to find objects and object relations.

  Note that this example programm will only work with small and medium sized
  OSM files, not with the planet.

  You can use the osmium_index example program to inspect the indexes.

  DEMONSTRATES USE OF:
  * file input
  * indexes and maps
  * use of the DiskStore handler
  * use of the ObjectRelations handler

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count
  * osmium_road_length
  * osmium_location_cache_create
  * osmium_location_cache_use

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cerrno>      // for errno
#include <cstdlib>     // for std::exit
#include <cstring>     // for std::strerror
#include <iostream>    // for std::cout, std::cerr
#include <string>      // for std::string
#include <sys/stat.h>  // for open
#include <sys/types.h> // for open

#ifdef _WIN32
# include <io.h>       // for _setmode
#endif

#ifdef _MSC_VER
# include <direct.h>
#endif

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// The DiskStore handler
#include <osmium/handler/disk_store.hpp>

// The ObjectRelations handler
#include <osmium/handler/object_relations.hpp>

// The indexes
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/multimap/sparse_mem_array.hpp>

using offset_index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, size_t>;
using map_type = osmium::index::multimap::SparseMemArray<osmium::unsigned_object_id_type, osmium::unsigned_object_id_type>;

/**
 * Small class wrapping index files, basically making sure errors are handled
 * and the files are closed on destruction.
 */
class IndexFile {

    int m_fd;

public:

    explicit IndexFile(const std::string& filename) :
        m_fd(::open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666)) { // NOLINT(hicpp-signed-bitwise)
        if (m_fd < 0) {
            std::cerr << "Can't open index file '" << filename << "': " << std::strerror(errno) << "\n";
            std::exit(2);
        }
#ifdef _WIN32
        _setmode(m_fd, _O_BINARY);
#endif
    }

    IndexFile(const IndexFile&) = delete;
    IndexFile& operator=(const IndexFile&) = delete;

    IndexFile(IndexFile&&) = delete;
    IndexFile& operator=(IndexFile&&) = delete;

    ~IndexFile() {
        if (m_fd >= 0) {
            close(m_fd);
        }
    }

    int fd() const noexcept {
        return m_fd;
    }

}; // class IndexFile

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE DIR\n";
        std::exit(2);
    }

    const std::string input_file_name{argv[1]};
    const std::string output_dir{argv[2]};

    // Create output directory. Ignore the error if it already exists.
#ifndef _WIN32
    const int result = ::mkdir(output_dir.c_str(), 0777);
#else
    const int result = mkdir(output_dir.c_str());
#endif
    if (result == -1 && errno != EEXIST) {
        std::cerr << "Problem creating directory '" << output_dir << "': " << std::strerror(errno) << "\n";
        std::exit(2);
    }

    // Create the output file which will contain our serialized OSM data
    const std::string data_file{output_dir + "/data.osm.ser"};
    const int data_fd = ::open(data_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666); // NOLINT(hicpp-signed-bitwise)
    if (data_fd < 0) {
        std::cerr << "Can't open data file '" << data_file << "': " << std::strerror(errno) << "\n";
        std::exit(2);
    }

#ifdef _WIN32
    _setmode(data_fd, _O_BINARY);
#endif

    // These indexes store the offset in the data file where each node, way,
    // or relation is stored.
    offset_index_type node_index;
    offset_index_type way_index;
    offset_index_type relation_index;

    // This handler will dump the internal data to disk using the given file
    // descriptor while updating the indexes.
    osmium::handler::DiskStore disk_store_handler{data_fd, node_index, way_index, relation_index};

    // These indexes store the mapping from node id to the ids of the ways
    // containing this node, and from node/way/relation ids to the ids of the
    // relations containing those objects.
    map_type map_node2way;
    map_type map_node2relation;
    map_type map_way2relation;
    map_type map_relation2relation;

    // This handler will update the map indexes.
    osmium::handler::ObjectRelations object_relations_handler{map_node2way, map_node2relation, map_way2relation, map_relation2relation};

    // Read OSM data buffer by buffer.
    osmium::io::Reader reader{input_file_name};

    while (osmium::memory::Buffer buffer = reader.read()) {
        // Write buffer to disk and update indexes.
        disk_store_handler(buffer);

        // Update object relation index maps.
        osmium::apply(buffer, object_relations_handler);
    }

    reader.close();

    // Write out node, way, and relation offset indexes to disk.
    IndexFile nodes_idx{output_dir + "/nodes.idx"};
    node_index.dump_as_list(nodes_idx.fd());

    IndexFile ways_idx{output_dir + "/ways.idx"};
    way_index.dump_as_list(ways_idx.fd());

    IndexFile relations_idx{output_dir + "/relations.idx"};
    relation_index.dump_as_list(relations_idx.fd());

    // Sort the maps (so later binary search will work on them) and write
    // them to disk.
    map_node2way.sort();
    IndexFile node2way_idx{output_dir + "/node2way.map"};
    map_node2way.dump_as_list(node2way_idx.fd());

    map_node2relation.sort();
    IndexFile node2relation_idx{output_dir + "/node2rel.map"};
    map_node2relation.dump_as_list(node2relation_idx.fd());

    map_way2relation.sort();
    IndexFile way2relation_idx{output_dir + "/way2rel.map"};
    map_way2relation.dump_as_list(way2relation_idx.fd());

    map_relation2relation.sort();
    IndexFile relation2relation_idx{output_dir + "/rel2rel.map"};
    map_relation2relation.dump_as_list(relation2relation_idx.fd());
}

