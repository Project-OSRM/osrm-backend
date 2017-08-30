#include "catch.hpp"

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/osm/node.hpp>

struct CallbackClass {

    int count = 0;

    void moving_in_buffer(size_t old_offset, size_t new_offset) {
        REQUIRE(old_offset > new_offset);
        ++count;
    }

}; // struct CallbackClass

TEST_CASE("Purge data from buffer") {

    constexpr size_t buffer_size = 10000;
    osmium::memory::Buffer buffer{buffer_size};

    SECTION("purge empty buffer") {
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 0);

        CallbackClass callback;
        buffer.purge_removed(&callback);

        REQUIRE(callback.count == 0);
        REQUIRE(buffer.committed() == 0);
    }

    SECTION("purge buffer with one object but nothing to delete") {
        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("testuser");
        }
        buffer.commit();
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 1);
        const size_t committed = buffer.committed();

        CallbackClass callback;
        buffer.purge_removed(&callback);

        REQUIRE(callback.count == 0);
        REQUIRE(committed == buffer.committed());
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 1);
    }

    SECTION("purge buffer with one object which gets deleted") {
        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("testuser");
            node_builder.set_removed(true);
        }
        buffer.commit();
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 1);

        CallbackClass callback;
        buffer.purge_removed(&callback);

        REQUIRE(callback.count == 0);
        REQUIRE(buffer.committed() == 0);
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 0);
    }

    SECTION("purge buffer with two objects, first gets deleted") {
        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("testuser");
            node_builder.set_removed(true);
        }
        buffer.commit();
        const size_t size1 = buffer.committed();
        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("testuser");
        }
        buffer.commit();
        const size_t size2 = buffer.committed() - size1;
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 2);

        CallbackClass callback;
        buffer.purge_removed(&callback);

        REQUIRE(callback.count == 1);
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 1);
        REQUIRE(buffer.committed() == size2);
    }

    SECTION("purge buffer with two objects, second gets deleted") {
        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("testuser_longer_name");
        }
        buffer.commit();
        size_t size1 = buffer.committed();
        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("testuser");
            node_builder.set_removed(true);
        }
        buffer.commit();

        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 2);

        CallbackClass callback;
        buffer.purge_removed(&callback);

        REQUIRE(callback.count == 0);
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 1);
        REQUIRE(buffer.committed() == size1);
    }

    SECTION("purge buffer with three objects, middle one gets deleted") {
        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("testuser_longer_name");
        }
        buffer.commit();

        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("testuser");
            node_builder.set_removed(true);
        }
        buffer.commit();

        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("sn");
        }
        buffer.commit();

        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 3);

        CallbackClass callback;
        buffer.purge_removed(&callback);

        REQUIRE(callback.count == 1);
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 2);
    }

    SECTION("purge buffer with three objects, all get deleted") {
        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("testuser_longer_name");
            node_builder.set_removed(true);
        }
        buffer.commit();

        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("testuser");
            node_builder.set_removed(true);
        }
        buffer.commit();

        {
            osmium::builder::NodeBuilder node_builder{buffer};
            node_builder.set_user("sn");
            node_builder.set_removed(true);
        }
        buffer.commit();

        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 3);

        CallbackClass callback;
        buffer.purge_removed(&callback);

        REQUIRE(callback.count == 0);
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 0);
    }

}
