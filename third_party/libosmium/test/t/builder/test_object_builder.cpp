#include "catch.hpp"

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm.hpp>

#include <string>

TEST_CASE("create objects using builder") {
    osmium::memory::Buffer buffer{1024 * 10};
    std::string user;

    SECTION("complete node with tags") {
        SECTION("user length 0") {
            user = "";
        }
        SECTION("user length 1") {
            user = "1";
        }
        SECTION("user length 2") {
            user = "12";
        }
        SECTION("user length 3") {
            user = "123";
        }
        SECTION("user length 4") {
            user = "1234";
        }
        SECTION("user length 5") {
            user = "12345";
        }
        SECTION("user length 6") {
            user = "123456";
        }
        SECTION("user length 7") {
            user = "1234567";
        }
        SECTION("user length 8") {
            user = "12345678";
        }
        SECTION("user length 9") {
            user = "123456789";
        }
        SECTION("user length 10") {
            user = "1234567890";
        }
        SECTION("user length 11") {
            user = "12345678901";
        }
        SECTION("user length 12") {
            user = "123456789012";
        }
        SECTION("user length 13") {
            user = "1234567890123";
        }
        SECTION("user length 14") {
            user = "12345678901234";
        }
        SECTION("user length 15") {
            user = "123456789012345";
        }
        SECTION("user length 16") {
            user = "1234567890123456";
        }
        SECTION("user length 17") {
            user = "12345678901234567";
        }
        SECTION("user length 18") {
            user = "123456789012345678";
        }

        const osmium::Location loc{1.2, 3.4};

        {
            osmium::builder::NodeBuilder builder{buffer};

            builder.set_id(17)
                .set_visible(true)
                .set_version(1)
                .set_changeset(123)
                .set_uid(555)
                .set_timestamp("2015-07-01T00:00:01Z")
                .set_location(loc)
                .set_user(user);

            builder.add_tags({{"highway", "primary"}, {"oneway", "yes"}});
        }

        const auto& node = buffer.get<osmium::Node>(buffer.commit());

        REQUIRE(node.id() == 17);
        REQUIRE(node.version() == 1);
        REQUIRE(node.changeset() == 123);
        REQUIRE(node.uid() == 555);
        REQUIRE(node.timestamp() == osmium::Timestamp{"2015-07-01T00:00:01Z"});
        REQUIRE(node.location() == loc);

        REQUIRE(user == node.user());

        REQUIRE(node.tags().size() == 2);
    }

    SECTION("complete way with tags") {
        SECTION("user length 0") {
            user = "";
        }
        SECTION("user length 1") {
            user = "1";
        }
        SECTION("user length 2") {
            user = "12";
        }
        SECTION("user length 3") {
            user = "123";
        }
        SECTION("user length 4") {
            user = "1234";
        }
        SECTION("user length 5") {
            user = "12345";
        }
        SECTION("user length 6") {
            user = "123456";
        }
        SECTION("user length 7") {
            user = "1234567";
        }
        SECTION("user length 8") {
            user = "12345678";
        }
        SECTION("user length 9") {
            user = "123456789";
        }
        SECTION("user length 10") {
            user = "1234567890";
        }
        SECTION("user length 11") {
            user = "12345678901";
        }
        SECTION("user length 12") {
            user = "123456789012";
        }
        SECTION("user length 13") {
            user = "1234567890123";
        }
        SECTION("user length 14") {
            user = "12345678901234";
        }
        SECTION("user length 15") {
            user = "123456789012345";
        }
        SECTION("user length 16") {
            user = "1234567890123456";
        }
        SECTION("user length 17") {
            user = "12345678901234567";
        }
        SECTION("user length 18") {
            user = "123456789012345678";
        }

        {
            osmium::builder::WayBuilder builder{buffer};

            builder.set_id(17)
                .set_visible(true)
                .set_version(1)
                .set_changeset(123)
                .set_uid(555)
                .set_timestamp("2015-07-01T00:00:01Z")
                .set_user(user);

            builder.add_tags({{"highway", "primary"}, {"oneway", "yes"}});
        }

        const auto& way = buffer.get<osmium::Way>(buffer.commit());

        REQUIRE(way.id() == 17);
        REQUIRE(way.version() == 1);
        REQUIRE(way.changeset() == 123);
        REQUIRE(way.uid() == 555);
        REQUIRE(way.timestamp() == osmium::Timestamp{"2015-07-01T00:00:01Z"});

        REQUIRE(user == way.user());

        REQUIRE(way.tags().size() == 2);
    }

    SECTION("complete relation with tags") {
        SECTION("user length 0") {
            user = "";
        }
        SECTION("user length 1") {
            user = "1";
        }
        SECTION("user length 2") {
            user = "12";
        }
        SECTION("user length 3") {
            user = "123";
        }
        SECTION("user length 4") {
            user = "1234";
        }
        SECTION("user length 5") {
            user = "12345";
        }
        SECTION("user length 6") {
            user = "123456";
        }
        SECTION("user length 7") {
            user = "1234567";
        }
        SECTION("user length 8") {
            user = "12345678";
        }
        SECTION("user length 9") {
            user = "123456789";
        }
        SECTION("user length 10") {
            user = "1234567890";
        }
        SECTION("user length 11") {
            user = "12345678901";
        }
        SECTION("user length 12") {
            user = "123456789012";
        }
        SECTION("user length 13") {
            user = "1234567890123";
        }
        SECTION("user length 14") {
            user = "12345678901234";
        }
        SECTION("user length 15") {
            user = "123456789012345";
        }
        SECTION("user length 16") {
            user = "1234567890123456";
        }
        SECTION("user length 17") {
            user = "12345678901234567";
        }
        SECTION("user length 18") {
            user = "123456789012345678";
        }

        {
            osmium::builder::RelationBuilder builder{buffer};

            builder.set_id(17)
                .set_visible(true)
                .set_version(1)
                .set_changeset(123)
                .set_uid(555)
                .set_timestamp("2015-07-01T00:00:01Z")
                .set_user(user);

            builder.add_tags({{"highway", "primary"}, {"oneway", "yes"}});
        }

        const auto& relation = buffer.get<osmium::Relation>(buffer.commit());

        REQUIRE(relation.id() == 17);
        REQUIRE(relation.version() == 1);
        REQUIRE(relation.changeset() == 123);
        REQUIRE(relation.uid() == 555);
        REQUIRE(relation.timestamp() == osmium::Timestamp{"2015-07-01T00:00:01Z"});

        REQUIRE(user == relation.user());

        REQUIRE(relation.tags().size() == 2);
    }

    SECTION("complete changeset with tags") {
        const osmium::Location bl{-1.2, -3.4};
        const osmium::Location tr{1.2, 3.4};

        SECTION("user length 0") {
            user = "";
        }
        SECTION("user length 1") {
            user = "1";
        }
        SECTION("user length 2") {
            user = "12";
        }
        SECTION("user length 3") {
            user = "123";
        }
        SECTION("user length 4") {
            user = "1234";
        }
        SECTION("user length 5") {
            user = "12345";
        }
        SECTION("user length 6") {
            user = "123456";
        }
        SECTION("user length 7") {
            user = "1234567";
        }
        SECTION("user length 8") {
            user = "12345678";
        }
        SECTION("user length 9") {
            user = "123456789";
        }
        SECTION("user length 10") {
            user = "1234567890";
        }
        SECTION("user length 11") {
            user = "12345678901";
        }
        SECTION("user length 12") {
            user = "123456789012";
        }
        SECTION("user length 13") {
            user = "1234567890123";
        }
        SECTION("user length 14") {
            user = "12345678901234";
        }
        SECTION("user length 15") {
            user = "123456789012345";
        }
        SECTION("user length 16") {
            user = "1234567890123456";
        }
        SECTION("user length 17") {
            user = "12345678901234567";
        }
        SECTION("user length 18") {
            user = "123456789012345678";
        }

        {
            osmium::builder::ChangesetBuilder builder{buffer};

            builder.set_id(17)
                .set_uid(222)
                .set_created_at(osmium::Timestamp{"2016-07-03T01:23:45Z"})
                .set_closed_at(osmium::Timestamp{"2016-07-03T01:23:48Z"})
                .set_num_changes(3)
                .set_num_comments(2)
                .set_bounds(osmium::Box{bl, tr})
                .set_user(user);
        }

        const auto& changeset = buffer.get<osmium::Changeset>(buffer.commit());

        REQUIRE(changeset.id() == 17);
        REQUIRE(changeset.uid() == 222);
        REQUIRE(changeset.created_at() == osmium::Timestamp{"2016-07-03T01:23:45Z"});
        REQUIRE(changeset.closed_at() == osmium::Timestamp{"2016-07-03T01:23:48Z"});
        REQUIRE(changeset.num_changes() == 3);
        REQUIRE(changeset.num_comments() == 2);

        const auto& box = changeset.bounds();
        REQUIRE(box.bottom_left() == bl);
        REQUIRE(box.top_right() == tr);

        REQUIRE(user == changeset.user());
    }

}

TEST_CASE("no call to set_user on node") {
    osmium::memory::Buffer buffer{1024 * 10};

    {
        osmium::builder::NodeBuilder builder{buffer};
    }

    const auto& node = buffer.get<osmium::Node>(buffer.commit());

    REQUIRE(*node.user() == '\0');
}

TEST_CASE("set_user with length on node") {
    osmium::memory::Buffer buffer{1024 * 10};
    std::string user = "userx";

    {
        osmium::builder::NodeBuilder builder{buffer};
        builder.set_user(user.c_str(), 4);
    }

    const auto& node = buffer.get<osmium::Node>(buffer.commit());

    REQUIRE(std::string{"user"} == node.user());
}

TEST_CASE("no call to set_user on way") {
    osmium::memory::Buffer buffer{1024 * 10};

    {
        osmium::builder::WayBuilder builder{buffer};
    }

    const auto& way = buffer.get<osmium::Way>(buffer.commit());

    REQUIRE(*way.user() == '\0');
}

TEST_CASE("set_user with length on way") {
    osmium::memory::Buffer buffer{1024 * 10};
    std::string user = "userx";

    {
        osmium::builder::WayBuilder builder{buffer};
        builder.set_user(user.c_str(), 4);
    }

    const auto& way = buffer.get<osmium::Way>(buffer.commit());

    REQUIRE(std::string{"user"} == way.user());
}

TEST_CASE("no call to set_user on changeset") {
    osmium::memory::Buffer buffer{1024 * 10};

    {
        osmium::builder::ChangesetBuilder builder{buffer};
    }

    const auto& changeset = buffer.get<osmium::Changeset>(buffer.commit());

    REQUIRE(*changeset.user() == '\0');
}

TEST_CASE("set_user with length on changeset") {
    osmium::memory::Buffer buffer{1024 * 10};
    std::string user = "userx";

    {
        osmium::builder::ChangesetBuilder builder{buffer};
        builder.set_user(user.c_str(), 4);
    }

    const auto& changeset = buffer.get<osmium::Changeset>(buffer.commit());

    REQUIRE(std::string{"user"} == changeset.user());
}

TEST_CASE("clear_user should clear the user field but nothing else") {
    osmium::memory::Buffer buffer{1024 * 10};
    std::string user = "user";

    {
        osmium::builder::NodeBuilder builder{buffer};
        builder.set_id(17)
            .set_visible(true)
            .set_version(1)
            .set_changeset(123)
            .set_uid(555)
            .set_timestamp("2015-07-01T00:00:01Z")
            .set_location(osmium::Location{1.2, 3.4})
            .set_user(user);
        builder.add_tags({{"highway", "primary"}, {"oneway", "yes"}});
    }

    auto& node = buffer.get<osmium::Node>(buffer.commit());

    REQUIRE(std::string{"user"} == node.user());

    node.clear_user();

    REQUIRE(std::string{""} == node.user());
    REQUIRE(node.uid() == 555);
    REQUIRE(node.tags().size() == 2);

    auto it = node.tags().begin();
    REQUIRE(it->key() == std::string{"highway"});
    REQUIRE(it->value() == std::string{"primary"});
    ++it;
    REQUIRE(it->key() == std::string{"oneway"});
    REQUIRE(it->value() == std::string{"yes"});
    ++it;
    REQUIRE(it == node.tags().end());
}

