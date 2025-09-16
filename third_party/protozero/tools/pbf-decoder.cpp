/*****************************************************************************

Protobuf decoder tool

Tool to decode unknown protocol buffer encoded messages. The protocol buffer
format doesn't contain enough information about the contents of a file to make
it decodable without the format description usually found in a `.proto` file,
so this tool does some informed guessing.

Usage:

    pbf-decoder [OPTIONS] [FILENAME]

Use "-" as a file name to read from STDIN.

The output always goes to STDOUT.

Call with --help/-h to see more options.

*****************************************************************************/

#include <protozero/pbf_reader.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <exception>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string decode(const char* data, std::size_t len, const std::string& indent);

// Try decoding as a nested message
bool decode_message(std::stringstream& out, const std::string& indent, const protozero::data_view view) {
    try {
        const auto nested = decode(view.data(), view.size(), indent + "  ");
        out << '\n' << nested;
        return true;
    } catch (const protozero::exception&) { // NOLINT(bugprone-empty-catch)
    }
    return false;
}

// Try decoding as a string (only printable characters allowed).
bool decode_printable_string(std::stringstream& out, const protozero::data_view view) {
    static constexpr const std::size_t max_string_length = 60;

    const std::string str{view.data(), view.size()};
    if (str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_:-") != std::string::npos) {
        return false;
    }

    if (str.size() > max_string_length) {
        out << '"' << str.substr(0, max_string_length) << "\"...\n";
    } else {
        out << '"' << str << '"' << '\n';
    }
    return true;
}

// Try decoding as a string.
bool decode_string(std::stringstream& out, const protozero::data_view view) {
    static constexpr const std::size_t max_string_length = 60;

    const std::string str{view.data(), std::min(view.size(), max_string_length)};
    out << '"';

    for (const auto c : str) {
        if (std::isprint(c) != 0) {
            out << c;
        } else {
            out << '.';
        }
    }

    out << '"' << '\n';

    return true;
}

// Print a list of numbers from a range
template <typename TRange>
void print_number_range(std::stringstream& out, const TRange& range) {
    bool first = true;
    for (auto val : range) {
        if (first) {
            first = false;
        } else {
            out << ',';
        }
        out << val;
    }
    out << '\n';
}

// Try decoding as packed repeated double
bool decode_packed_double(std::stringstream& out, std::size_t size, protozero::pbf_reader& message) {
    if (size % 8 != 0) {
        return false;
    }
    try {
        print_number_range(out, message.get_packed_double());
        return true;
    } catch (const protozero::exception&) { // NOLINT(bugprone-empty-catch)
    }

    return false;
}

// Try decoding as packed repeated float
bool decode_packed_float(std::stringstream& out, std::size_t size, protozero::pbf_reader& message) {
    if (size % 4 != 0) {
        return false;
    }
    try {
        print_number_range(out, message.get_packed_float());
        return true;
    } catch (const protozero::exception&) { // NOLINT(bugprone-empty-catch)
    }

    return false;
}

// Try decoding as packed repeated varint
bool decode_packed_varint(std::stringstream& out, protozero::pbf_reader& message) {
    try {
        print_number_range(out, message.get_packed_int64());
        return true;
    } catch (const protozero::exception&) { // NOLINT(bugprone-empty-catch)
    }

    return false;
}

std::string decode(const char* data, std::size_t len, const std::string& indent) {
    std::stringstream stream;
    protozero::pbf_reader message{data, len};
    while (message.next()) {
        stream << indent << message.tag() << ": ";
        switch (message.wire_type()) {
            case protozero::pbf_wire_type::varint: {
                // This is int32, int64, uint32, uint64, sint32, sint64, bool, or enum.
                // Try decoding as int64.
                stream << message.get_int64() << '\n';
                break;
            }
            case protozero::pbf_wire_type::fixed64:
                // This is fixed64, sfixed64, or double.
                // Try decoding as a double, because int64_t or uint64_t
                // would probably be encoded as varint.
                stream << message.get_double() << '\n';
                break;
            case protozero::pbf_wire_type::length_delimited: {
                // This is string, bytes, embedded messages, or packed repeated fields.
                protozero::pbf_reader message_copy{message};
                const auto view = message.get_view();

                decode_message(stream, indent, view) ||
                    decode_printable_string(stream, view) ||
                    decode_packed_double(stream, view.size(), message_copy) ||
                    decode_packed_float(stream, view.size(), message_copy) ||
                    decode_packed_varint(stream, message_copy) ||
                    decode_string(stream, view);
                break;
            }
            case protozero::pbf_wire_type::fixed32:
                // This is fixed32, sfixed32, or float.
                // Try decoding as a float, because int32_t or uint32_t
                // would probably be encoded as varint.
                stream << message.get_float() << '\n';
                break;
            default:
                throw protozero::unknown_pbf_wire_type_exception{};
        }
    }

    return stream.str();
}

void print_help() {
    std::cout << "Usage: pbf-decoder [OPTIONS] [INPUT_FILE]\n\n"
              << "Dump raw contents of protobuf encoded file.\n"
              << "To read from STDIN use '-' as INPUT_FILE.\n"
              << "\nOptions:\n"
              << "  -h, --help           This help message\n"
              << "  -l, --length=LENGTH  Read only LENGTH bytes\n"
              << "  -o, --offset=OFFSET  Start reading from OFFSET bytes\n";
}

std::vector<char> read_from_file(const char* filename) {
    const std::ifstream file{filename, std::ios::binary};
    return std::vector<char>{std::istreambuf_iterator<char>(file.rdbuf()),
                             std::istreambuf_iterator<char>()};
}

std::vector<char> read_from_stdin() {
    return std::vector<char>{std::istreambuf_iterator<char>(std::cin.rdbuf()),
                             std::istreambuf_iterator<char>()};
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help",         no_argument, nullptr, 'h'},
        {"length", required_argument, nullptr, 'l'},
        {"offset", required_argument, nullptr, 'o'},
        {nullptr, 0, nullptr, 0}
    };

    std::size_t offset = 0;
    std::size_t length = std::numeric_limits<std::size_t>::max();

    while (true) {
        const int c = getopt_long(argc, argv, "hl:o:", long_options, nullptr); // NOLINT(concurrency-mt-unsafe) no threads here
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                return 0;
            case 'l':
                length = std::atoll(optarg); // NOLINT(cert-err34-c)
                                             // good enough for a limited-use tool
                break;
            case 'o':
                offset = std::atoll(optarg); // NOLINT(cert-err34-c)
                                             // good enough for a limited-use tool
                break;
            default:
                return 1;
        }
    }

    const int remaining_args = argc - optind;
    if (remaining_args != 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INPUT_FILE]\n\n"
                  << "Call with --help/-h to see options.\n";
        return 1;
    }

    const std::string filename{argv[optind]};

    try {
        std::vector<char> buffer{filename == "-" ? read_from_stdin() :
                                                   read_from_file(argv[optind])};

        if (offset > buffer.size()) {
            throw std::runtime_error{"offset is larger than file size"};
        }

        if (offset > 0) {
            buffer.erase(buffer.begin(), buffer.begin() + offset);
        }

        if (length < buffer.size()) {
            buffer.resize(length);
        }

        std::cout << decode(buffer.data(), buffer.size(), "");
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    return 0;
}

