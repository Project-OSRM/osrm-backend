#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <test.hpp>

#include <fstream>
#include <stdexcept>
#include <string>

bool got_an_assert = false;

std::string load_test_tile() {
    std::string path{"data/mapbox-streets-v6-14-8714-8017.mvt"};
    std::ifstream stream{path, std::ios_base::in|std::ios_base::binary};
    if (!stream.is_open()) {
        throw std::runtime_error{"could not open: '" + path + "'"};
    }

    const std::string message{std::istreambuf_iterator<char>(stream.rdbuf()),
                              std::istreambuf_iterator<char>()};

    stream.close();
    return message;
}

