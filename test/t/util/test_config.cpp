#include "catch.hpp"

#include <cstdlib>
#include <string>

namespace osmium {

    namespace detail {

        const char* env = nullptr;
        std::string name;

        inline const char* getenv_wrapper(const char* var) noexcept {
            name = var;
            return env;
        }

    } // namespace detail

} // namespace osmium

#define OSMIUM_TEST_RUNNER
#include <osmium/util/config.hpp>

TEST_CASE("get_pool_threads") {
    osmium::detail::env = nullptr;
    REQUIRE(osmium::config::get_pool_threads() == 0);
    REQUIRE(osmium::detail::name == "OSMIUM_POOL_THREADS");
    osmium::detail::env = "";
    REQUIRE(osmium::config::get_pool_threads() == 0);
    osmium::detail::env = "2";
    REQUIRE(osmium::config::get_pool_threads() == 2);
}

TEST_CASE("use_pool_threads_for_pbf_parsing") {
    osmium::detail::env = nullptr;
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    REQUIRE(osmium::detail::name == "OSMIUM_USE_POOL_THREADS_FOR_PBF_PARSING");
    osmium::detail::env = "";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());

    osmium::detail::env = "off";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());
    osmium::detail::env = "OFF";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());
    osmium::detail::env = "false";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());
    osmium::detail::env = "no";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());
    osmium::detail::env = "No";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());
    osmium::detail::env = "0";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());

    osmium::detail::env = "on";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    osmium::detail::env = "ON";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    osmium::detail::env = "true";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    osmium::detail::env = "yes";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    osmium::detail::env = "Yes";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    osmium::detail::env = "1";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
}

TEST_CASE("get_max_queue_size") {
    osmium::detail::env = nullptr;
    REQUIRE(osmium::config::get_max_queue_size("NAME", 0) == 0);
    REQUIRE(osmium::detail::name == "OSMIUM_MAX_NAME_QUEUE_SIZE");

    REQUIRE(osmium::config::get_max_queue_size("NAME", 7) == 7);

    osmium::detail::env = "";
    REQUIRE(osmium::config::get_max_queue_size("NAME", 7) == 7);
    osmium::detail::env = "0";
    REQUIRE(osmium::config::get_max_queue_size("NAME", 7) == 7);
    osmium::detail::env = "3";
    REQUIRE(osmium::config::get_max_queue_size("NAME", 7) == 3);
}

