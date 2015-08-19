#include "catch.hpp"

#include <osmium/io/detail/string_table.hpp>

TEST_CASE("String store") {
    osmium::io::detail::StringStore ss(100);

    SECTION("empty") {
        REQUIRE(ss.begin() == ss.end());
    }

    SECTION("add zero-length string") {
        const char* s1 = ss.add("");
        REQUIRE(std::string(s1) == "");

        auto it = ss.begin();
        REQUIRE(s1 == *it);
        REQUIRE(std::string(*it) == "");
        REQUIRE(++it == ss.end());
    }

    SECTION("add strings") {
        const char* s1 = ss.add("foo");
        const char* s2 = ss.add("bar");
        REQUIRE(s1 != s2);
        REQUIRE(std::string(s1) == "foo");
        REQUIRE(std::string(s2) == "bar");

        auto it = ss.begin();
        REQUIRE(s1 == *it++);
        REQUIRE(s2 == *it++);
        REQUIRE(it == ss.end());
    }

    SECTION("add zero-length string and longer strings") {
        const char* s1 = ss.add("");
        const char* s2 = ss.add("xxx");
        const char* s3 = ss.add("yyyyy");

        auto it = ss.begin();
        REQUIRE(std::string(*it++) == "");
        REQUIRE(std::string(*it++) == "xxx");
        REQUIRE(std::string(*it++) == "yyyyy");
        REQUIRE(it == ss.end());
    }

    SECTION("add many strings") {
        for (const char* teststring : {"a", "abc", "abcd", "abcde"}) {
            int i = 0;
            for (; i < 100; ++i) {
                ss.add(teststring);
            }

            for (const char* s : ss) {
                REQUIRE(std::string(s) == teststring);
                --i;
            }

            REQUIRE(i == 0);
            ss.clear();
        }
    }

}

TEST_CASE("String table") {
    osmium::io::detail::StringTable st;

    SECTION("empty") {
        REQUIRE(st.size() == 1);
        REQUIRE(std::next(st.begin()) == st.end());
    }

    SECTION("add strings") {
        REQUIRE(st.add("foo") == 1);
        REQUIRE(st.add("bar") == 2);
        REQUIRE(st.add("bar") == 2);
        REQUIRE(st.add("baz") == 3);
        REQUIRE(st.add("foo") == 1);
        REQUIRE(st.size() == 4);

        auto it = st.begin();
        REQUIRE(std::string("") == *it++);
        REQUIRE(std::string("foo") == *it++);
        REQUIRE(std::string("bar") == *it++);
        REQUIRE(std::string("baz") == *it++);
        REQUIRE(it == st.end());

        st.clear();
        REQUIRE(st.size() == 1);
    }

}

