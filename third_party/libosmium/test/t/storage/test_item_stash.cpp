#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/storage/item_stash.hpp>

#include <sstream>
#include <string>
#include <vector>

osmium::memory::Buffer generate_test_data() {
    using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

    osmium::memory::Buffer buffer{1024 * 1024, osmium::memory::Buffer::auto_grow::yes};

    const osmium::object_id_type num_nodes     = 100;
    const osmium::object_id_type num_ways      =  50;
    const osmium::object_id_type num_relations =  30;

    osmium::object_id_type id = 1;
    for (; id <= num_nodes; ++id) {
        osmium::builder::add_node(buffer, _id(id));
    }

    for (; id <= num_nodes + num_ways; ++id) {
        osmium::builder::add_way(buffer, _id(id));
    }

    for (; id <= num_nodes + num_ways + num_relations; ++id) {
        osmium::builder::add_relation(buffer, _id(id));
    }

    return buffer;
}


TEST_CASE("Item stash handle") {
    const auto handle = osmium::ItemStash::handle_type{};
    REQUIRE_FALSE(handle.valid());

    std::stringstream ss;
    ss << handle;
    REQUIRE(ss.str() == "-");
}

TEST_CASE("Item stash") {
    const auto buffer = generate_test_data();

    osmium::ItemStash stash;
    REQUIRE(stash.size() == 0);
    REQUIRE(stash.count_removed() == 0);

    std::vector<osmium::ItemStash::handle_type> handles;
    for (const auto& item : buffer) {
        auto handle = stash.add_item(item);
        handles.push_back(handle);
    }

    REQUIRE(stash.size() == 180);
    REQUIRE(stash.count_removed() == 0);

    REQUIRE(stash.used_memory() > 1024 * 1024);

    osmium::object_id_type id = 1;
    for (auto& handle : handles) { // must be reference because we will change it!
        REQUIRE(handle.valid());
        const auto& item = stash.get_item(handle);
        bool correct_type = item.type() == osmium::item_type::node ||
                            item.type() == osmium::item_type::way ||
                            item.type() == osmium::item_type::relation;
        REQUIRE(correct_type);
        const auto& obj = static_cast<const osmium::OSMObject&>(item);
        REQUIRE(obj.id() == id);

        std::stringstream ss;
        ss << handle;
        REQUIRE(ss.str() == std::to_string(id));

        if (obj.id() % 3 == 0) {
            stash.remove_item(handle);
            handle = osmium::ItemStash::handle_type{};
        }

        ++id;
    }

    REQUIRE(stash.size() == 120);
    REQUIRE(stash.count_removed() == 60);

    id = 1;
    int count_valid   = 0;
    int count_invalid = 0;
    for (auto handle : handles) {
        if (handle.valid()) {
            ++count_valid;
            const auto& item = stash.get_item(handle);
            const bool correct_type = item.type() == osmium::item_type::node ||
                                      item.type() == osmium::item_type::way ||
                                      item.type() == osmium::item_type::relation;
            REQUIRE(correct_type);
            const auto& obj = static_cast<const osmium::OSMObject&>(item);
            REQUIRE(obj.id() == id);
        } else {
            ++count_invalid;
        }
        ++id;
    }

    REQUIRE(count_valid   == 120);
    REQUIRE(count_invalid ==  60);

    stash.garbage_collect();
    REQUIRE(stash.size() == 120);
    REQUIRE(stash.count_removed() == 0);

    id = 1;
    for (auto handle : handles) {
        if (handle.valid()) {
            const auto& item = stash.get_item(handle);
            const bool correct_type = item.type() == osmium::item_type::node ||
                                      item.type() == osmium::item_type::way ||
                                      item.type() == osmium::item_type::relation;
            REQUIRE(correct_type);
            const auto& obj = static_cast<const osmium::OSMObject&>(item);
            REQUIRE(obj.id() == id);
        }
        ++id;
    }

    stash.clear();
    REQUIRE(stash.size() == 0);
    REQUIRE(stash.count_removed() == 0);
}

TEST_CASE("Fill item stash until it garbage collects") {
    const auto buffer = generate_test_data();

    osmium::ItemStash stash;
    REQUIRE(stash.size() == 0);
    REQUIRE(stash.count_removed() == 0);

    const auto& node = buffer.get<osmium::Node>(0);

    std::vector<osmium::ItemStash::handle_type> handles;
    std::size_t num_items = 6 * 1000 * 1000;
    for (std::size_t i = 0; i < num_items; ++i) {
        auto handle = stash.add_item(node);
        handles.push_back(handle);
    }

    REQUIRE(stash.size() == num_items);
    REQUIRE(stash.count_removed() == 0);

    for (std::size_t i = 0; i < num_items; ++i) {
        if (i % 10 != 0) {
            stash.remove_item(handles[i]);
        }
    }

    REQUIRE(stash.size() == num_items / 10);
    REQUIRE(stash.count_removed() == num_items / 10 * 9);

    // trigger compaction
    stash.add_item(node);

    REQUIRE(stash.size() == num_items / 10 + 1);
    REQUIRE(stash.count_removed() == 0);
}

