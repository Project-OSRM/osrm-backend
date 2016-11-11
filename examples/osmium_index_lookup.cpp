/*

  EXAMPLE osmium_index

  Example program to look at Osmium indexes on disk.

  You can use the osmium_dump_internal example program to create the offset
  indexes or osmium_location_cache_create to create a node location index.

  DEMONSTRATES USE OF:
  * access to indexes on disk

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count
  * osmium_road_length
  * osmium_location_cache_create
  * osmium_location_cache_use

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <algorithm>   // for std::all_of, std::equal_range
#include <cstdlib>     // for std::exit
#include <fcntl.h>     // for open
#include <getopt.h>    // for getopt_long
#include <iostream>    // for std::cout, std::cerr
#include <memory>      // for std::unique_ptr
#include <string>      // for std::string
#include <sys/stat.h>  // for open
#include <sys/types.h> // for open
#include <vector>      // for std::vector

// Disk-based indexes
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/sparse_file_array.hpp>

// osmium::Location
#include <osmium/osm/location.hpp>

// Basic Osmium types
#include <osmium/osm/types.hpp>

// Virtual class for disk index access. If offers functions to dump the
// indexes and to search for ids in the index.
template <typename TValue>
class IndexAccess {

    int m_fd;

public:

    IndexAccess(int fd) :
        m_fd(fd) {
    }

    int fd() const noexcept {
        return m_fd;
    }

    virtual ~IndexAccess() = default;

    virtual void dump() const = 0;

    virtual bool search(const osmium::unsigned_object_id_type& key) const = 0;

    bool search(const std::vector<osmium::unsigned_object_id_type>& keys) const {
        return std::all_of(keys.cbegin(), keys.cend(), [this](const osmium::unsigned_object_id_type& key) {
            return search(key);
        });
    }

}; // class IndexAccess

// Implementation of IndexAccess for dense indexes usually used for very large
// extracts or the planet.
template <typename TValue>
class IndexAccessDense : public IndexAccess<TValue> {

    using index_type  = typename osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, TValue>;

public:

    IndexAccessDense(int fd) :
        IndexAccess<TValue>(fd) {
    }

    void dump() const override {
        index_type index{this->fd()};

        for (std::size_t i = 0; i < index.size(); ++i) {
            if (index.get(i) != TValue{}) {
                std::cout << i << " " << index.get(i) << "\n";
            }
        }
    }

    bool search(const osmium::unsigned_object_id_type& key) const override {
        index_type index{this->fd()};

        try {
            TValue value = index.get(key);
            std::cout << key << " " << value << "\n";
        } catch (...) {
            std::cout << key << " not found\n";
            return false;
        }

        return true;
    }

}; // class IndexAccessDense

// Implementation of IndexAccess for sparse indexes usually used for small or
// medium sized extracts or for "multimap" type indexes.
template <typename TValue>
class IndexAccessSparse : public IndexAccess<TValue> {

    using index_type = typename osmium::index::map::SparseFileArray<osmium::unsigned_object_id_type, TValue>;

public:

    IndexAccessSparse(int fd) :
        IndexAccess<TValue>(fd) {
    }

    void dump() const override {
        index_type index{this->fd()};

        for (const auto& element : index) {
            std::cout << element.first << " " << element.second << "\n";
        }
    }

    bool search(const osmium::unsigned_object_id_type& key) const override {
        using element_type = typename index_type::element_type;
        index_type index{this->fd()};

        element_type elem{key, TValue{}};
        const auto positions = std::equal_range(index.begin(),
                                                index.end(),
                                                elem,
                                                [](const element_type& lhs,
                                                   const element_type& rhs) {
            return lhs.first < rhs.first;
        });
        if (positions.first == positions.second) {
            std::cout << key << " not found\n";
            return false;
        }

        for (auto it = positions.first; it != positions.second; ++it) {
            std::cout << it->first << " " << it->second << "\n";
        }

        return true;
    }

}; // class IndexAccessSparse

// This class contains the code to parse the command line arguments, check
// them and present the results to the rest of the program in an easy-to-use
// way.
class Options {

    std::vector<osmium::unsigned_object_id_type> m_ids;
    std::string m_type;
    std::string m_filename;
    bool m_dump = false;
    bool m_array_format = false;
    bool m_list_format = false;

    void print_help() {
        std::cout << "Usage: osmium_index_lookup [OPTIONS]\n\n"
                  << "-h, --help        Print this help message\n"
                  << "-a, --array=FILE  Read given index file in array format\n"
                  << "-l, --list=FILE   Read given index file in list format\n"
                  << "-d, --dump        Dump contents of index file to STDOUT\n"
                  << "-s, --search=ID   Search for given id (Option can appear multiple times)\n"
                  << "-t, --type=TYPE   Type of value ('location', 'id', or 'offset')\n"
        ;
    }

public:

    Options(int argc, char* argv[]) {
        if (argc == 1) {
            print_help();
            std::exit(1);
        }

        static struct option long_options[] = {
            {"array",  required_argument, 0, 'a'},
            {"dump",         no_argument, 0, 'd'},
            {"help",         no_argument, 0, 'h'},
            {"list",   required_argument, 0, 'l'},
            {"search", required_argument, 0, 's'},
            {"type",   required_argument, 0, 't'},
            {0, 0, 0, 0}
        };

        while (true) {
            const int c = getopt_long(argc, argv, "a:dhl:s:t:", long_options, 0);
            if (c == -1) {
                break;
            }

            switch (c) {
                case 'a':
                    m_array_format = true;
                    m_filename = optarg;
                    break;
                case 'd':
                    m_dump = true;
                    break;
                case 'h':
                    print_help();
                    std::exit(0);
                case 'l':
                    m_list_format = true;
                    m_filename = optarg;
                    break;
                case 's':
                    m_ids.push_back(std::atoll(optarg));
                    break;
                case 't':
                    m_type = optarg;
                    if (m_type != "location" && m_type != "id" && m_type != "offset") {
                        std::cerr << "Unknown type '" << m_type
                                  << "'. Must be 'location', 'id', or 'offset'.\n";
                        std::exit(2);
                    }
                    break;
                default:
                    std::exit(2);
            }
        }

        if (m_array_format == m_list_format) {
            std::cerr << "Need option --array or --list, but not both\n";
            std::exit(2);
        }

        if (m_dump && !m_ids.empty()) {
            std::cerr << "Need option --dump or --search, but not both\n";
            std::exit(2);
        }

        if (m_type.empty()) {
            std::cerr << "Need --type argument.\n";
            std::exit(2);
        }

    }

    const char* filename() const noexcept {
        return m_filename.c_str();
    }

    bool dense_format() const noexcept {
        return m_array_format;
    }

    bool do_dump() const noexcept {
        return m_dump;
    }

    const std::vector<osmium::unsigned_object_id_type>& search_keys() const noexcept {
        return m_ids;
    }

    bool type_is(const char* type) const noexcept {
        return m_type == type;
    }

}; // class Options


// Factory function to create the right IndexAccess-derived class.
template <typename TValue>
std::unique_ptr<IndexAccess<TValue>> create(bool dense, int fd) {
    std::unique_ptr<IndexAccess<TValue>> ptr;

    if (dense) {
        ptr.reset(new IndexAccessDense<TValue>{fd});
    } else {
        ptr.reset(new IndexAccessSparse<TValue>{fd});
    }

    return ptr;
}

// Do the actual work: Either dump the index or search in the index.
template <typename TValue>
int run(const IndexAccess<TValue>& index, const Options& options) {
    if (options.do_dump()) {
        index.dump();
        return 0;
    } else {
        return index.search(options.search_keys()) ? 0 : 1;
    }
}

int main(int argc, char* argv[]) {
    // Parse command line options.
    Options options{argc, argv};

    // Open the index file.
    const int fd = open(options.filename(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Can not open file '" << options.filename()
                  << "': " << std::strerror(errno) << '\n';
        std::exit(2);
    }

    // Depending on the type of index, we have different implementations.
    if (options.type_is("location")) {
        // index id -> location
        const auto index = create<osmium::Location>(options.dense_format(), fd);
        return run(*index, options);
    } else if (options.type_is("id")) {
        // index id -> id
        const auto index = create<osmium::unsigned_object_id_type>(options.dense_format(), fd);
        return run(*index, options);
    } else {
        // index id -> offset
        const auto index = create<std::size_t>(options.dense_format(), fd);
        return run(*index, options);
    }
}

