#include "catch.hpp"

#include <boost/crc.hpp>

#include <osmium/osm/changeset.hpp>
#include <osmium/osm/crc.hpp>

#include "helper.hpp"

TEST_CASE("Basic Changeset") {

    osmium::CRC<boost::crc_32_type> crc32;

    osmium::memory::Buffer buffer(10 * 1000);

    osmium::Changeset& cs1 = buffer_add_changeset(buffer,
        "user",
        {{"comment", "foo"}});

    cs1.set_id(42)
       .set_created_at(100)
       .set_closed_at(200)
       .set_num_changes(7)
       .set_num_comments(3)
       .set_uid(9);

    REQUIRE(42 == cs1.id());
    REQUIRE(9 == cs1.uid());
    REQUIRE(7 == cs1.num_changes());
    REQUIRE(3 == cs1.num_comments());
    REQUIRE(true == cs1.closed());
    REQUIRE(osmium::Timestamp(100) == cs1.created_at());
    REQUIRE(osmium::Timestamp(200) == cs1.closed_at());
    REQUIRE(1 == cs1.tags().size());
    REQUIRE(std::string("user") == cs1.user());

    crc32.update(cs1);
    REQUIRE(crc32().checksum() == 0x502e8c0e);

    osmium::Changeset& cs2 = buffer_add_changeset(buffer,
        "user",
        {{"comment", "foo"}, {"foo", "bar"}});

    cs2.set_id(43)
       .set_created_at(120)
       .set_num_changes(21)
       .set_num_comments(osmium::num_comments_type(0))
       .set_uid(9);

    REQUIRE(43 == cs2.id());
    REQUIRE(9 == cs2.uid());
    REQUIRE(21 == cs2.num_changes());
    REQUIRE(0 == cs2.num_comments());
    REQUIRE(false == cs2.closed());
    REQUIRE(osmium::Timestamp(120) == cs2.created_at());
    REQUIRE(osmium::Timestamp() == cs2.closed_at());
    REQUIRE(2 == cs2.tags().size());
    REQUIRE(std::string("user") == cs2.user());

    REQUIRE(cs1 != cs2);

    REQUIRE(cs1 < cs2);
    REQUIRE(cs1 <= cs2);
    REQUIRE(false == (cs1 > cs2));
    REQUIRE(false == (cs1 >= cs2));

}

TEST_CASE("Create changeset without helper") {
    osmium::memory::Buffer buffer(10 * 1000);
    osmium::builder::ChangesetBuilder builder(buffer);

    osmium::Changeset& cs1 = builder.object();
    cs1.set_id(42)
       .set_created_at(100)
       .set_closed_at(200)
       .set_num_changes(7)
       .set_num_comments(2)
       .set_uid(9);

    builder.add_user("user");
    add_tags(buffer, builder, {
        {"key1", "val1"},
        {"key2", "val2"}
    });

    {
        osmium::builder::ChangesetDiscussionBuilder disc_builder(buffer, &builder);
        disc_builder.add_comment(osmium::Timestamp(300), 10, "user2");
        disc_builder.add_comment_text("foo");
        disc_builder.add_comment(osmium::Timestamp(400), 9, "user");
        disc_builder.add_comment_text("bar");
    }

    buffer.commit();

    REQUIRE(42 == cs1.id());
    REQUIRE(9 == cs1.uid());
    REQUIRE(7 == cs1.num_changes());
    REQUIRE(2 == cs1.num_comments());
    REQUIRE(true == cs1.closed());
    REQUIRE(osmium::Timestamp(100) == cs1.created_at());
    REQUIRE(osmium::Timestamp(200) == cs1.closed_at());
    REQUIRE(2 == cs1.tags().size());
    REQUIRE(std::string("user") == cs1.user());

    auto cit = cs1.discussion().begin();

    REQUIRE(cit != cs1.discussion().end());
    REQUIRE(cit->date() == osmium::Timestamp(300));
    REQUIRE(cit->uid() == 10);
    REQUIRE(std::string("user2") == cit->user());
    REQUIRE(std::string("foo") == cit->text());

    REQUIRE(++cit != cs1.discussion().end());
    REQUIRE(cit->date() == osmium::Timestamp(400));
    REQUIRE(cit->uid() == 9);
    REQUIRE(std::string("user") == cit->user());
    REQUIRE(std::string("bar") == cit->text());

    REQUIRE(++cit == cs1.discussion().end());
}

