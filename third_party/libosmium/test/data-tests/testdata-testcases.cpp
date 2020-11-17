
#define CATCH_CONFIG_RUNNER

#include "testdata-testcases.hpp"

#include <iostream>
#include <string>

std::string dirname;

int main(int argc, char* argv[]) {
    const char* testcases_dir = getenv("TESTCASES_DIR");
    if (testcases_dir) {
        dirname = testcases_dir;
        std::cerr << "Running tests from '" << dirname << "' (from TESTCASES_DIR environment variable)\n";
    } else {
        std::cerr << "Please set TESTCASES_DIR environment variable.\n";
        std::exit(1);
    }

    try {
        return Catch::Session().run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        std::exit(1);
    }
}

