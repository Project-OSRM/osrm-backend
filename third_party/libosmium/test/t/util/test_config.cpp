#include "catch.hpp"

#include <cstdlib>

const char* env = nullptr;
std::string name;

const char* fake_getenv(const char* env_var) {
    name = env_var;
    return env;
}

#define getenv fake_getenv

#include <osmium/util/config.hpp>

TEST_CASE("get_pool_threads") {
    env = nullptr;
    REQUIRE(osmium::config::get_pool_threads() == 0);
    REQUIRE(name == "OSMIUM_POOL_THREADS");
    env = "";
    REQUIRE(osmium::config::get_pool_threads() == 0);
    env = "2";
    REQUIRE(osmium::config::get_pool_threads() == 2);
}

TEST_CASE("use_pool_threads_for_pbf_parsing") {
    env = nullptr;
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    REQUIRE(name == "OSMIUM_USE_POOL_THREADS_FOR_PBF_PARSING");
    env = "";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());

    env = "off";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());
    env = "OFF";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());
    env = "false";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());
    env = "no";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());
    env = "No";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());
    env = "0";
    REQUIRE_FALSE(osmium::config::use_pool_threads_for_pbf_parsing());

    env = "on";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    env = "ON";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    env = "true";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    env = "yes";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    env = "Yes";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
    env = "1";
    REQUIRE(osmium::config::use_pool_threads_for_pbf_parsing());
}

TEST_CASE("get_max_queue_size") {
    env = nullptr;
    REQUIRE(osmium::config::get_max_queue_size("NAME", 0) == 0);
    REQUIRE(name == "OSMIUM_MAX_NAME_QUEUE_SIZE");

    REQUIRE(osmium::config::get_max_queue_size("NAME", 7) == 7);

    env = "";
    REQUIRE(osmium::config::get_max_queue_size("NAME", 7) == 7);
    env = "0";
    REQUIRE(osmium::config::get_max_queue_size("NAME", 7) == 7);
    env = "3";
    REQUIRE(osmium::config::get_max_queue_size("NAME", 7) == 3);
}

