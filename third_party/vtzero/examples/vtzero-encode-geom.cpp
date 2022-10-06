/*****************************************************************************

  Example program for vtzero library.

  vtzero-encode-geom - Encode geometry on command line

  This can be used for debugging. Uses internals of vtzero!

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/geometry.hpp>

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int64_t get_int(const char* arg) {
    char* endptr = nullptr;
    const int64_t value = std::strtoll(arg, &endptr, 10);

    if (*endptr == '\0') {
        return value;
    }

    throw std::runtime_error{"not a valid number"};
}

uint32_t move_to(const char* arg) {
    if (!std::isdigit(arg[0])) {
        throw std::runtime_error{"need count after M command"};
    }

    const auto scount = get_int(arg);
    if (scount <= 0) {
        throw std::runtime_error{"count after M command must be 1 or larger"};
    }
    const auto count = static_cast<uint32_t>(scount);
    std::cout << "MOVE_TO(" << count << ")\t" << vtzero::detail::command_move_to(count) << '\n';

    return vtzero::detail::command_move_to(count);
}

uint32_t line_to(const char* arg) {
    if (!std::isdigit(arg[0])) {
        throw std::runtime_error{"need count after L command"};
    }

    const auto scount = get_int(arg);
    if (scount <= 0) {
        throw std::runtime_error{"count after L command must be 1 or larger"};
    }
    const auto count = static_cast<uint32_t>(scount);
    std::cout << "LINE_TO(" << count << ")\t" << vtzero::detail::command_line_to(count) << '\n';

    return vtzero::detail::command_line_to(count);
}

uint32_t close_path(const char* arg) {
    if (arg[0] != '\0') {
        throw std::runtime_error{"extra data after C command"};
    }
    std::cout << "CLOSE_PATH\t" << vtzero::detail::command_close_path() << '\n';

    return vtzero::detail::command_close_path();
}

uint32_t number(const char* arg) {
    const auto num = static_cast<int32_t>(get_int(arg));
    std::cout << "number(" << num << ")\t" << protozero::encode_zigzag32(num) << '\n';

    return protozero::encode_zigzag32(num);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " GEOMETRY ELEMENTS...\n"
                  << "GEOMETRY ELEMENTS are:\n"
                  << "  M[count] -- MOVE_TO count\n"
                  << "  L[count] -- LINE_TO count\n"
                  << "  C        -- CLOSE_PATH\n"
                  << "  [number] -- number that will be zigzag encoded\n";
        return 1;
    }

    std::vector<uint32_t> values;

    std::cout << "raw data\tencoded\n-----------------------------------\n";
    for (int i = 1; i < argc; ++i) {
        try {
            switch (argv[i][0]) {
                case '\0':
                    break;
                case 'M':
                    values.push_back(move_to(argv[i] + 1));
                    break;
                case 'L':
                    values.push_back(line_to(argv[i] + 1));
                    break;
                case 'C':
                    values.push_back(close_path(argv[i] + 1));
                    break;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    values.push_back(number(argv[i]));
                    break;
                default:
                    throw std::runtime_error{std::string{"unknown data: "} + argv[i]};
                    return 1;
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "error(" << i << "): " << e.what() << '\n';
            return 1;
        }
    }

    std::string out{"["};

    for (auto value : values) {
        out += ' ';
        out += std::to_string(value);
        out += ',';
    }

    out.back() = ' ';

    std::cout << '\n' << out << "]\n";
}

