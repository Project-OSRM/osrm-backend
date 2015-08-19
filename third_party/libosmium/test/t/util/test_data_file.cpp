#include "catch.hpp"

#include <cstring>

#include <osmium/util/data_file.hpp>

TEST_CASE("temporary file") {

    SECTION("create/open") {
        osmium::util::DataFile file;

        REQUIRE(!!file);
        int fd = file.fd();

        REQUIRE(fd > 0);

        const char buf[] = "foobar";
        REQUIRE(::write(fd, buf, sizeof(buf)) == sizeof(buf));

        file.close();

        REQUIRE(!file);
    }

}

TEST_CASE("named file") {

    SECTION("create/open") {
        {
            osmium::util::DataFile file("test.data", true);

            REQUIRE(!!file);
            int fd = file.fd();

            REQUIRE(fd > 0);

            REQUIRE(file.size() == 0);

            const char buf[] = "foobar";
            REQUIRE(::write(fd, buf, sizeof(buf) - 1) == sizeof(buf) - 1);

            file.close();

            REQUIRE(!file);
        }
        {
            osmium::util::DataFile file("test.data", false);

            REQUIRE(!!file);
            int fd = file.fd();

            REQUIRE(fd > 0);

            REQUIRE(file.size() == 6);

            char buf[10];
            int len = ::read(fd, buf, sizeof(buf));

            REQUIRE(len == 6);
            REQUIRE(!strncmp(buf, "foobar", 6));

            file.close();

            REQUIRE(!file);
            REQUIRE(unlink("test.data") == 0);
        }
    }

    SECTION("grow file") {
        osmium::util::DataFile file("test.data", true);

        REQUIRE(!!file);

        REQUIRE(file.size() == 0);
        file.grow(10);
        REQUIRE(file.size() == 10);
    }

}

