/*

  Example program to look at Osmium indexes on disk.

  The code in this example file is released into the Public Domain.

*/

#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include <getopt.h>

#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>

template <typename TKey, typename TValue>
class IndexSearch {

    typedef typename osmium::index::map::DenseFileArray<TKey, TValue> dense_index_type;
    typedef typename osmium::index::map::SparseFileArray<TKey, TValue> sparse_index_type;

    int m_fd;
    bool m_dense_format;

    void dump_dense() {
        dense_index_type index(m_fd);

        for (std::size_t i = 0; i < index.size(); ++i) {
            if (index.get(i) != TValue()) {
                std::cout << i << " " << index.get(i) << "\n";
            }
        }
    }

    void dump_sparse() {
        sparse_index_type index(m_fd);

        for (auto& element : index) {
            std::cout << element.first << " " << element.second << "\n";
        }
    }

    bool search_dense(TKey key) {
        dense_index_type index(m_fd);

        try {
            TValue value = index.get(key);
            std::cout << key << " " << value << "\n";
        } catch (...) {
            std::cout << key << " not found\n";
            return false;
        }

        return true;
    }

    bool search_sparse(TKey key) {
        typedef typename sparse_index_type::element_type element_type;
        sparse_index_type index(m_fd);

        element_type elem {key, TValue()};
        auto positions = std::equal_range(index.begin(), index.end(), elem, [](const element_type& lhs, const element_type& rhs) {
            return lhs.first < rhs.first;
        });
        if (positions.first == positions.second) {
            std::cout << key << " not found\n";
            return false;
        }

        for (auto& it = positions.first; it != positions.second; ++it) {
            std::cout << it->first << " " << it->second << "\n";
        }

        return true;
    }

public:

    IndexSearch(int fd, bool dense_format) :
        m_fd(fd),
        m_dense_format(dense_format) {
    }

    void dump() {
        if (m_dense_format) {
            dump_dense();
        } else {
            dump_sparse();
        }
    }

    bool search(TKey key) {
        if (m_dense_format) {
            return search_dense(key);
        } else {
            return search_sparse(key);
        }
    }

    bool search(const std::vector<TKey>& keys) {
        bool found_all = true;

        for (const auto key : keys) {
            if (!search(key)) {
                found_all = false;
            }
        }

        return found_all;
    }

}; // class IndexSearch

enum return_code : int {
    okay      = 0,
    not_found = 1,
    error     = 2,
    fatal     = 3
};

class Options {

    std::vector<osmium::unsigned_object_id_type> m_ids;
    std::string m_type;
    std::string m_filename;
    bool m_dump = false;
    bool m_array_format = false;
    bool m_list_format = false;

    void print_help() {
        std::cout << "Usage: osmium_index [OPTIONS]\n\n"
                  << "-h, --help        Print this help message\n"
                  << "-a, --array=FILE  Read given index file in array format\n"
                  << "-l, --list=FILE   Read given index file in list format\n"
                  << "-d, --dump        Dump contents of index file to STDOUT\n"
                  << "-s, --search=ID   Search for given id (Option can appear multiple times)\n"
                  << "-t, --type=TYPE   Type of value ('location' or 'offset')\n"
        ;
    }

public:

    Options(int argc, char* argv[]) {
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
            int c = getopt_long(argc, argv, "a:dhl:s:t:", long_options, 0);
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
                    std::exit(return_code::okay);
                case 'l':
                    m_list_format = true;
                    m_filename = optarg;
                    break;
                case 's':
                    m_ids.push_back(std::atoll(optarg));
                    break;
                case 't':
                    m_type = optarg;
                    if (m_type != "location" && m_type != "offset") {
                        std::cerr << "Unknown type '" << m_type << "'. Must be 'location' or 'offset'.\n";
                        std::exit(return_code::fatal);
                    }
                    break;
                default:
                    std::exit(return_code::fatal);
            }
        }

        if (m_array_format == m_list_format) {
            std::cerr << "Need option --array or --list, but not both\n";
            std::exit(return_code::fatal);
        }

        if (m_type.empty()) {
            std::cerr << "Need --type argument.\n";
            std::exit(return_code::fatal);
        }

    }

    const std::string& filename() const noexcept {
        return m_filename;
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

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    Options options(argc, argv);

    std::cout << std::fixed << std::setprecision(7);
    int fd = open(options.filename().c_str(), O_RDWR);

    bool result_okay = true;

    if (options.type_is("location")) {
        IndexSearch<osmium::unsigned_object_id_type, osmium::Location> is(fd, options.dense_format());

        if (options.do_dump()) {
            is.dump();
        } else {
            result_okay = is.search(options.search_keys());
        }
    } else {
        IndexSearch<osmium::unsigned_object_id_type, size_t> is(fd, options.dense_format());

        if (options.do_dump()) {
            is.dump();
        } else {
            result_okay = is.search(options.search_keys());
        }
    }

    std::exit(result_okay ? return_code::okay : return_code::not_found);
}

