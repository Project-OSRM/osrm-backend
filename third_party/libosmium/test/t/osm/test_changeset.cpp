#include "catch.hpp"

#include "test_crc.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/osm/changeset.hpp>
#include <osmium/osm/crc.hpp>

#include <string>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

TEST_CASE("Build changeset") {
    osmium::memory::Buffer buffer{10 * 1000};

    osmium::builder::add_changeset(buffer,
        _cid(42),
        _created_at(time_t(100)),
        _closed_at(time_t(200)),
        _num_changes(7),
        _num_comments(3),
        _uid(9),
        _user("user"),
        _tag("comment", "foo")
    );

    const osmium::Changeset& cs1 = buffer.get<osmium::Changeset>(0);

    REQUIRE(42 == cs1.id());
    REQUIRE(9 == cs1.uid());
    REQUIRE(7 == cs1.num_changes());
    REQUIRE(3 == cs1.num_comments());
    REQUIRE(cs1.closed());
    REQUIRE(osmium::Timestamp{100} == cs1.created_at());
    REQUIRE(osmium::Timestamp{200} == cs1.closed_at());
    REQUIRE(1 == cs1.tags().size());
    REQUIRE(std::string("user") == cs1.user());

    osmium::CRC<crc_type> crc32;
    crc32.update(cs1);
    REQUIRE(crc32().checksum() == 0x502e8c0e);

    const auto pos = osmium::builder::add_changeset(buffer,
        _cid(43),
        _created_at(time_t(120)),
        _num_changes(21),
        _num_comments(0),
        _uid(9),
        _user("user"),
        _tag("comment", "foo"),
        _tag("foo", "bar"),
        _comment({time_t(300), 10, "user2", "foo"}),
        _comments({{time_t(400), 9, "user", "bar"}})
    );

    const osmium::Changeset& cs2 = buffer.get<osmium::Changeset>(pos);

    REQUIRE(43 == cs2.id());
    REQUIRE(9 == cs2.uid());
    REQUIRE(21 == cs2.num_changes());
    REQUIRE(0 == cs2.num_comments());
    REQUIRE_FALSE(cs2.closed());
    REQUIRE(osmium::Timestamp{120} == cs2.created_at());
    REQUIRE(osmium::Timestamp{} == cs2.closed_at());
    REQUIRE(2 == cs2.tags().size());
    REQUIRE(std::string("user") == cs2.user());

    REQUIRE(cs1 != cs2);

    REQUIRE(cs1 < cs2);
    REQUIRE(cs1 <= cs2);
    REQUIRE_FALSE(cs1 > cs2);
    REQUIRE_FALSE(cs1 >= cs2);

    auto cit = cs2.discussion().begin();

    REQUIRE(cit != cs2.discussion().end());
    REQUIRE(cit->date() == osmium::Timestamp{300});
    REQUIRE(cit->uid() == 10);
    REQUIRE(std::string("user2") == cit->user());
    REQUIRE(std::string("foo") == cit->text());

    REQUIRE(++cit != cs2.discussion().end());
    REQUIRE(cit->date() == osmium::Timestamp{400});
    REQUIRE(cit->uid() == 9);
    REQUIRE(std::string("user") == cit->user());
    REQUIRE(std::string("bar") == cit->text());

    REQUIRE(++cit == cs2.discussion().end());
}

TEST_CASE("Create changeset without helper") {
    osmium::memory::Buffer buffer{10 * 1000};
    {
        osmium::builder::ChangesetBuilder builder{buffer};

        builder.set_id(42)
            .set_created_at(100)
            .set_closed_at(200)
            .set_num_changes(7)
            .set_num_comments(2)
            .set_uid(9)
            .set_user("user");

        {
            osmium::builder::TagListBuilder tl_builder{builder};
            tl_builder.add_tag("key1", "val1");
            tl_builder.add_tag("key2", "val2");
        }

        osmium::builder::ChangesetDiscussionBuilder disc_builder{builder};
        disc_builder.add_comment(osmium::Timestamp{300}, 10, "user2");
        disc_builder.add_comment_text("foo");
        disc_builder.add_comment(osmium::Timestamp{400}, 9, "user");
        disc_builder.add_comment_text("bar");
    }

    const auto& cs = buffer.get<osmium::Changeset>(buffer.commit());

    REQUIRE(42 == cs.id());
    REQUIRE(9 == cs.uid());
    REQUIRE(7 == cs.num_changes());
    REQUIRE(2 == cs.num_comments());
    REQUIRE(cs.closed());
    REQUIRE(osmium::Timestamp{100} == cs.created_at());
    REQUIRE(osmium::Timestamp{200} == cs.closed_at());
    REQUIRE(2 == cs.tags().size());
    REQUIRE(std::string("user") == cs.user());

    auto cit = cs.discussion().begin();

    REQUIRE(cit != cs.discussion().end());
    REQUIRE(cit->date() == osmium::Timestamp{300});
    REQUIRE(cit->uid() == 10);
    REQUIRE(std::string("user2") == cit->user());
    REQUIRE(std::string("foo") == cit->text());

    REQUIRE(++cit != cs.discussion().end());
    REQUIRE(cit->date() == osmium::Timestamp{400});
    REQUIRE(cit->uid() == 9);
    REQUIRE(std::string("user") == cit->user());
    REQUIRE(std::string("bar") == cit->text());

    REQUIRE(++cit == cs.discussion().end());
}

TEST_CASE("Change changeset") {
    osmium::memory::Buffer buffer{10 * 1000};

    osmium::builder::add_changeset(buffer,
        _cid(42),
        _created_at(time_t(100)),
        _closed_at(time_t(200)),
        _num_changes(7),
        _num_comments(3),
        _uid(9),
        _user("user"),
        _tag("comment", "foo")
    );

    auto& cs = buffer.get<osmium::Changeset>(0);

    cs.set_id(12);
    cs.set_created_at(time_t(200));
    cs.set_closed_at(time_t(300));
    cs.set_num_changes(3);
    cs.set_num_comments(4);
    cs.set_uid(10);
    cs.clear_user();

    REQUIRE(12 == cs.id());
    REQUIRE(10 == cs.uid());
    REQUIRE(3 == cs.num_changes());
    REQUIRE(4 == cs.num_comments());
    REQUIRE(cs.closed());
    REQUIRE(osmium::Timestamp{200} == cs.created_at());
    REQUIRE(osmium::Timestamp{300} == cs.closed_at());
    REQUIRE(1 == cs.tags().size());
    REQUIRE(std::string("") == cs.user());
    REQUIRE(cs.tags().size() == 1);

    auto it = cs.tags().begin();
    REQUIRE(it->key() == std::string{"comment"});
    REQUIRE(it->value() == std::string{"foo"});
    ++it;
    REQUIRE(it == cs.tags().end());
}

