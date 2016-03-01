#include "catch.hpp"

#include <osmium/util/file.hpp>

#ifdef _WIN32
#include <crtdbg.h>
// https://msdn.microsoft.com/en-us/library/ksazx244.aspx
// https://msdn.microsoft.com/en-us/library/a9yf33zb.aspx
class DoNothingInvalidParameterHandler {

    static void invalid_parameter_handler(
                    const wchar_t* expression,
                    const wchar_t* function,
                    const wchar_t* file,
                    unsigned int line,
                    uintptr_t pReserved
                ) {
        // do nothing
    }

    _invalid_parameter_handler old_handler;

public:

    DoNothingInvalidParameterHandler() :
        old_handler(_set_invalid_parameter_handler(invalid_parameter_handler)) {
        _CrtSetReportMode(_CRT_ASSERT, 0);
    }

    ~DoNothingInvalidParameterHandler() {
        _set_invalid_parameter_handler(old_handler);
    }

}; // class InvalidParameterHandler
#endif


TEST_CASE("file_size") {

#ifdef _WIN32
    DoNothingInvalidParameterHandler handler;
#endif

    SECTION("illegal fd should throw") {
        REQUIRE_THROWS_AS(osmium::util::file_size(-1), std::system_error);
    }

    SECTION("unused fd should throw") {
        // its unlikely that fd 1000 is open...
        REQUIRE_THROWS_AS(osmium::util::file_size(1000), std::system_error);
    }

}

TEST_CASE("resize_file") {

#ifdef _WIN32
    DoNothingInvalidParameterHandler handler;
#endif

    SECTION("illegal fd should throw") {
        REQUIRE_THROWS_AS(osmium::util::resize_file(-1, 10), std::system_error);
    }

    SECTION("unused fd should throw") {
        // its unlikely that fd 1000 is open...
        REQUIRE_THROWS_AS(osmium::util::resize_file(1000, 10), std::system_error);
    }

}

