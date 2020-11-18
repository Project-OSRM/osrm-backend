#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/detail/opl_input_format.hpp>
#include <osmium/io/opl_input.hpp>
#include <osmium/opl.hpp>

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

namespace oid = osmium::io::detail;

TEST_CASE("Parse OPL: base exception") {
    const osmium::opl_error e{"foo"};
    REQUIRE(e.data == nullptr);
    REQUIRE(e.line == 0);
    REQUIRE(e.column == 0);
    REQUIRE(e.msg == "OPL error: foo");
    REQUIRE(std::string{e.what()} == "OPL error: foo");
}

TEST_CASE("Parse OPL: exception with line and column") {
    const char* d = "data";
    osmium::opl_error e{"bar", d};
    e.set_pos(17, 23);
    REQUIRE(e.data == d);
    REQUIRE(e.line == 17);
    REQUIRE(e.column == 23);
    REQUIRE(e.msg == "OPL error: bar on line 17 column 23");
    REQUIRE(std::string{e.what()} == "OPL error: bar on line 17 column 23");
}

TEST_CASE("Parse OPL: space") {
    const std::string d{"a b \t c"};

    const char* s = d.data();
    REQUIRE_THROWS_AS(oid::opl_parse_space(&s), const osmium::opl_error&);

    s = d.data() + 1;
    oid::opl_parse_space(&s);
    REQUIRE(*s == 'b');

    REQUIRE_THROWS_AS(oid::opl_parse_space(&s), const osmium::opl_error&);

    ++s;
    oid::opl_parse_space(&s);
    REQUIRE(*s == 'c');
}

TEST_CASE("Parse OPL: check for space") {
    REQUIRE(oid::opl_non_empty("aaa"));
    REQUIRE(oid::opl_non_empty("a b"));
    REQUIRE_FALSE(oid::opl_non_empty(" "));
    REQUIRE_FALSE(oid::opl_non_empty(" x"));
    REQUIRE_FALSE(oid::opl_non_empty("\tx"));
    REQUIRE_FALSE(oid::opl_non_empty(""));
}

TEST_CASE("Parse OPL: skip section") {
    const std::string d{"abcd efgh"};
    const char* skip1 = d.data() + 4;
    const char* skip2 = d.data() + 9;
    const char* s = d.data();
    REQUIRE(oid::opl_skip_section(&s) == skip1);
    REQUIRE(s == skip1);
    ++s;
    REQUIRE(oid::opl_skip_section(&s) == skip2);
    REQUIRE(s == skip2);
}

TEST_CASE("Parse OPL: parse escaped") {
    std::string result;

    SECTION("Empty string") {
        const char* s = "";
        REQUIRE_THROWS_WITH(oid::opl_parse_escaped(&s, result),
                            "OPL error: eol");
    }

    SECTION("Illegal character for hex") {
        const char* s = "x";
        REQUIRE_THROWS_WITH(oid::opl_parse_escaped(&s, result),
                            "OPL error: not a hex char");
    }

    SECTION("Illegal character for hex after legal hex characters") {
        const char* s = "0123x";
        REQUIRE_THROWS_WITH(oid::opl_parse_escaped(&s, result),
                            "OPL error: not a hex char");
    }

    SECTION("Too long") {
        const char* s = "123456780";
        REQUIRE_THROWS_WITH(oid::opl_parse_escaped(&s, result),
                            "OPL error: hex escape too long");
    }

    SECTION("No data") {
        const char* s = "%";
        const char* e = s + std::strlen(s);
        oid::opl_parse_escaped(&s, result);
        REQUIRE(result.size() == 1);
        REQUIRE(result[0] == '\0');
        REQUIRE(s == e);
    }

    SECTION("One hex char") {
        const char* s = "9%";
        const char* e = s + std::strlen(s);
        oid::opl_parse_escaped(&s, result);
        REQUIRE(result.size() == 1);
        REQUIRE(result == "\t");
        REQUIRE(s == e);
    }

    SECTION("Two hex chars (lowercase)") {
        const char* s = "3c%";
        const char* e = s + std::strlen(s);
        oid::opl_parse_escaped(&s, result);
        REQUIRE(result.size() == 1);
        REQUIRE(result == "<");
        REQUIRE(s == e);
    }

    SECTION("Two hex char (uppercase)") {
        const char* s = "3C%";
        const char* e = s + std::strlen(s);
        oid::opl_parse_escaped(&s, result);
        REQUIRE(result.size() == 1);
        REQUIRE(result == "<");
        REQUIRE(s == e);
    }

    SECTION("Longer unicode characters") {
        const char* s1 = "30dc%";
        oid::opl_parse_escaped(&s1, result);
        result.append("_");
        const char* s2 = "1d11e%";
        oid::opl_parse_escaped(&s2, result);
        result.append("_");
        const char* s3 = "1f6eb%";
        oid::opl_parse_escaped(&s3, result);
        REQUIRE(result == u8"\u30dc_\U0001d11e_\U0001f6eb");
    }

    SECTION("Data after %") {
        const char* s = "5a%abc";
        oid::opl_parse_escaped(&s, result);
        REQUIRE(result.size() == 1);
        REQUIRE(result == "Z");
        REQUIRE(std::string{s} == "abc");
    }

}

TEST_CASE("Parse OPL: parse string") {
    std::string result;

    SECTION("empty string") {
        const char* s = "";
        const char* e = s + std::strlen(s);
        oid::opl_parse_string(&s, result);
        REQUIRE(result.empty());
        REQUIRE(s == e);
    }

    SECTION("normal string") {
        const char* s = "foo";
        const char* e = s + std::strlen(s);
        oid::opl_parse_string(&s, result);
        REQUIRE(result.size() == 3);
        REQUIRE(result == "foo");
        REQUIRE(s == e);
    }

    SECTION("string with space") {
        const char* s = "foo bar";
        const char* e = s + 3;
        oid::opl_parse_string(&s, result);
        REQUIRE(result.size() == 3);
        REQUIRE(result == "foo");
        REQUIRE(s == e);
    }

    SECTION("string with tab") {
        const char* s = "foo\tbar";
        const char* e = s + 3;
        oid::opl_parse_string(&s, result);
        REQUIRE(result.size() == 3);
        REQUIRE(result == "foo");
        REQUIRE(s == e);
    }

    SECTION("string with comma") {
        const char* s = "foo,bar";
        const char* e = s + 3;
        oid::opl_parse_string(&s, result);
        REQUIRE(result.size() == 3);
        REQUIRE(result == "foo");
        REQUIRE(s == e);
    }

    SECTION("string with equal sign") {
        const char* s = "foo=bar";
        const char* e = s + 3;
        oid::opl_parse_string(&s, result);
        REQUIRE(result.size() == 3);
        REQUIRE(result == "foo");
        REQUIRE(s == e);
    }

    SECTION("string with escaped characters") {
        const char* s = "foo%3d%bar";
        const char* e = s + std::strlen(s);
        oid::opl_parse_string(&s, result);
        REQUIRE(result.size() == 7);
        REQUIRE(result == "foo=bar");
        REQUIRE(s == e);
    }

    SECTION("string with escaped characters at end") {
        const char* s = "foo%3d%";
        const char* e = s + std::strlen(s);
        oid::opl_parse_string(&s, result);
        REQUIRE(result.size() == 4);
        REQUIRE(result == "foo=");
        REQUIRE(s == e);
    }

    SECTION("string with invalid escaping") {
        const char* s = "foo%";
        REQUIRE_THROWS_WITH(oid::opl_parse_string(&s, result),
                            "OPL error: eol");
    }

    SECTION("string with invalid escaped characters") {
        const char* s = "foo%x%";
        REQUIRE_THROWS_WITH(oid::opl_parse_string(&s, result),
                            "OPL error: not a hex char");
    }

}

template <typename T = int64_t>
T test_parse_int(const char* s) {
    const auto r = oid::opl_parse_int<T>(&s);
    REQUIRE(*s == 'x');
    return r;
}

TEST_CASE("Parse OPL: integer") {
    REQUIRE(test_parse_int("0x") == 0);
    REQUIRE(test_parse_int("-0x") == 0);
    REQUIRE(test_parse_int("1x") == 1);
    REQUIRE(test_parse_int("17x") == 17);
    REQUIRE(test_parse_int("-1x") == -1);
    REQUIRE(test_parse_int("1234567890123x") == 1234567890123);
    REQUIRE(test_parse_int("-1234567890123x") == -1234567890123);

    REQUIRE_THROWS_WITH(test_parse_int(""),
                        "OPL error: expected integer");

    REQUIRE_THROWS_WITH(test_parse_int("-x"),
                        "OPL error: expected integer");

    REQUIRE_THROWS_WITH(test_parse_int(" 1"),
                        "OPL error: expected integer");

    REQUIRE_THROWS_WITH(test_parse_int("x"),
                        "OPL error: expected integer");

    REQUIRE_THROWS_WITH(test_parse_int("99999999999999999999999x"),
                        "OPL error: integer too long");
}

TEST_CASE("Parse OPL: int32_t") {
    REQUIRE(test_parse_int<int32_t>("0x") == 0);
    REQUIRE(test_parse_int<int32_t>("123x") == 123);
    REQUIRE(test_parse_int<int32_t>("-123x") == -123);

    REQUIRE_THROWS_WITH(test_parse_int<int32_t>("12345678901x"),
                        "OPL error: integer too long");
    REQUIRE_THROWS_WITH(test_parse_int<int32_t>("-12345678901x"),
                        "OPL error: integer too long");
}

TEST_CASE("Parse OPL: uint32_t") {
    REQUIRE(test_parse_int<uint32_t>("0x") == 0);
    REQUIRE(test_parse_int<uint32_t>("123x") == 123);

    REQUIRE_THROWS_WITH(test_parse_int<uint32_t>("-123x"),
                        "OPL error: integer too long");

    REQUIRE_THROWS_WITH(test_parse_int<uint32_t>("12345678901x"),
                        "OPL error: integer too long");

    REQUIRE_THROWS_WITH(test_parse_int<uint32_t>("-12345678901x"),
                        "OPL error: integer too long");
}

TEST_CASE("Parse OPL: visible flag") {
    const char* data = "V";
    const char* e = data + std::strlen(data);
    REQUIRE(oid::opl_parse_visible(&data));
    REQUIRE(e == data);

}

TEST_CASE("Parse OPL: deleted flag") {
    const char* data = "D";
    const char* e = data + std::strlen(data);
    REQUIRE_FALSE(oid::opl_parse_visible(&data));
    REQUIRE(e == data);
}

TEST_CASE("Parse OPL: invalid visible flag") {
    const char* data = "x";
    REQUIRE_THROWS_WITH(oid::opl_parse_visible(&data),
                        "OPL error: invalid visible flag");
}

TEST_CASE("Parse OPL: timestamp (empty)") {
    const char* data = "";
    const char* e = data + std::strlen(data);
    REQUIRE(oid::opl_parse_timestamp(&data) == osmium::Timestamp{});
    REQUIRE(e == data);
}

TEST_CASE("Parse OPL: timestamp (space)") {
    const char* data = " ";
    const char* e = data;
    REQUIRE(oid::opl_parse_timestamp(&data) == osmium::Timestamp{});
    REQUIRE(e == data);
}

TEST_CASE("Parse OPL: timestamp (tab)") {
    const char* data = "\t";
    const char* e = data;
    REQUIRE(oid::opl_parse_timestamp(&data) == osmium::Timestamp{});
    REQUIRE(e == data);
}

TEST_CASE("Parse OPL: timestamp (invalid)") {
    const char* data = "abc";
    REQUIRE_THROWS_WITH(oid::opl_parse_timestamp(&data),
                        "OPL error: can not parse timestamp");
}

TEST_CASE("Parse OPL: timestamp (valid)") {
    const char* data = "2016-03-04T17:28:03Z";
    const char* e = data + std::strlen(data);
    REQUIRE(oid::opl_parse_timestamp(&data) == osmium::Timestamp{"2016-03-04T17:28:03Z"});
    REQUIRE(e == data);
}

TEST_CASE("Parse OPL: valid timestamp with trailing data") {
    const char* data = "2016-03-04T17:28:03Zxxx";
    REQUIRE(oid::opl_parse_timestamp(&data) == osmium::Timestamp{"2016-03-04T17:28:03Z"});
    REQUIRE(std::string{data} == "xxx");
}

TEST_CASE("Parse OPL: tags") {
    osmium::memory::Buffer buffer{1024};

    SECTION("Empty") {
        const char* data = "";
        REQUIRE_THROWS_WITH(oid::opl_parse_tags(data, buffer),
                            "OPL error: expected '='");
    }

    SECTION("One tag") {
        const char* data = "foo=bar";
        oid::opl_parse_tags(data, buffer);
        const auto& taglist = buffer.get<osmium::TagList>(0);
        REQUIRE(taglist.size() == 1);
        REQUIRE(std::string{taglist.begin()->key()} == "foo");
        REQUIRE(std::string{taglist.begin()->value()} == "bar");
    }

    SECTION("Empty key and value are allowed") {
        const char* data = "=";
        oid::opl_parse_tags(data, buffer);
        const auto& taglist = buffer.get<osmium::TagList>(0);
        REQUIRE(taglist.size() == 1);
        REQUIRE(std::string{taglist.begin()->key()}.empty());
        REQUIRE(std::string{taglist.begin()->value()}.empty());
    }

    SECTION("Multiple tags") {
        const char* data = "highway=residential,oneway=yes,maxspeed=30";
        oid::opl_parse_tags(data, buffer);
        const auto& taglist = buffer.get<osmium::TagList>(0);
        REQUIRE(taglist.size() == 3);
        auto it = taglist.cbegin();
        REQUIRE(std::string{it->key()} == "highway");
        REQUIRE(std::string{it->value()} == "residential");
        ++it;
        REQUIRE(std::string{it->key()} == "oneway");
        REQUIRE(std::string{it->value()} == "yes");
        ++it;
        REQUIRE(std::string{it->key()} == "maxspeed");
        REQUIRE(std::string{it->value()} == "30");
        ++it;
        REQUIRE(it == taglist.cend());
    }

    SECTION("No equal signs") {
        const char* data = "a";
        REQUIRE_THROWS_WITH(oid::opl_parse_tags(data, buffer),
                            "OPL error: expected '='");
    }

    SECTION("Two equal signs") {
        const char* data = "a=b=c";
        REQUIRE_THROWS_WITH(oid::opl_parse_tags(data, buffer),
                            "OPL error: expected ','");
    }

}

TEST_CASE("Parse OPL: nodes") {
    osmium::memory::Buffer buffer{1024};

    SECTION("Empty") {
        const char* const s = "";
        oid::opl_parse_way_nodes(s, s + std::strlen(s), buffer);
        REQUIRE(buffer.written() == 0);
    }

    SECTION("Invalid format, missing n") {
        const char* const s = "xyz";
        REQUIRE_THROWS_WITH(oid::opl_parse_way_nodes(s, s + std::strlen(s), buffer),
                            "OPL error: expected 'n'");
    }

    SECTION("Invalid format, missing ID") {
        const char* const s = "nx";
        REQUIRE_THROWS_WITH(oid::opl_parse_way_nodes(s, s + std::strlen(s), buffer),
                            "OPL error: expected integer");
    }

    SECTION("Valid format: one node") {
        const char* const s = "n123";
        oid::opl_parse_way_nodes(s, s + std::strlen(s), buffer);
        REQUIRE(buffer.written() > 0);
        const auto& wnl = buffer.get<osmium::WayNodeList>(0);
        REQUIRE(wnl.size() == 1);
        REQUIRE(wnl.begin()->ref() == 123);
    }

    SECTION("Valid format: two nodes") {
        const char* const s = "n123,n456";
        oid::opl_parse_way_nodes(s, s + std::strlen(s), buffer);
        REQUIRE(buffer.written() > 0);
        const auto& wnl = buffer.get<osmium::WayNodeList>(0);
        REQUIRE(wnl.size() == 2);
        auto it = wnl.begin();
        REQUIRE(it->ref() == 123);
        ++it;
        REQUIRE(it->ref() == 456);
        ++it;
        REQUIRE(it == wnl.end());
    }

    SECTION("Trailing comma") {
        const char* const s = "n123,n456,";
        oid::opl_parse_way_nodes(s, s + std::strlen(s), buffer);
        REQUIRE(buffer.written() > 0);
        const auto& wnl = buffer.get<osmium::WayNodeList>(0);
        REQUIRE(wnl.size() == 2);
        auto it = wnl.begin();
        REQUIRE(it->ref() == 123);
        ++it;
        REQUIRE(it->ref() == 456);
        ++it;
        REQUIRE(it == wnl.end());
    }

    SECTION("Way nodes with coordinates") {
        const char* const s = "n123x1.2y3.4,n456x33y0.1";
        oid::opl_parse_way_nodes(s, s + std::strlen(s), buffer);
        REQUIRE(buffer.written() > 0);
        const auto& wnl = buffer.get<osmium::WayNodeList>(0);
        REQUIRE(wnl.size() == 2);
        auto it = wnl.begin();
        REQUIRE(it->ref() == 123);
        const osmium::Location loc1{1.2, 3.4};
        REQUIRE(it->location() == loc1);
        ++it;
        REQUIRE(it->ref() == 456);
        const osmium::Location loc2{33.0, 0.1};
        REQUIRE(it->location() == loc2);
        ++it;
        REQUIRE(it == wnl.end());
    }

}

TEST_CASE("Parse OPL: members") {
    osmium::memory::Buffer buffer{1024};

    SECTION("Empty") {
        const char* const s = "";
        oid::opl_parse_relation_members(s, s + std::strlen(s), buffer);
        REQUIRE(buffer.written() == 0);
    }

    SECTION("Invalid: unknown object type") {
        const char* const s = "x123@foo";
        REQUIRE_THROWS_WITH(oid::opl_parse_relation_members(s, s + std::strlen(s), buffer),
                            "OPL error: unknown object type");
    }

    SECTION("Invalid: illegal ref") {
        const char* const s = "wx";
        REQUIRE_THROWS_WITH(oid::opl_parse_relation_members(s, s + std::strlen(s), buffer),
                            "OPL error: expected integer");
    }

    SECTION("Invalid: missing @") {
        const char* const s = "n123foo";
        REQUIRE_THROWS_WITH(oid::opl_parse_relation_members(s, s + std::strlen(s), buffer),
                            "OPL error: expected '@'");
    }

    SECTION("Valid format: one member") {
        const char* const s = "n123@foo";
        oid::opl_parse_relation_members(s, s + std::strlen(s), buffer);
        REQUIRE(buffer.written() > 0);
        const auto& rml = buffer.get<osmium::RelationMemberList>(0);
        REQUIRE(rml.size() == 1);
        auto it = rml.begin();
        REQUIRE(it->type() == osmium::item_type::node);
        REQUIRE(it->ref() == 123);
        REQUIRE(std::string{it->role()} == "foo");
        ++it;
        REQUIRE(it == rml.end());
    }

    SECTION("Valid format: one member without role") {
        const char* const s = "n123@";
        oid::opl_parse_relation_members(s, s + std::strlen(s), buffer);
        REQUIRE(buffer.written() > 0);
        const auto& rml = buffer.get<osmium::RelationMemberList>(0);
        REQUIRE(rml.size() == 1);
        auto it = rml.begin();
        REQUIRE(it->type() == osmium::item_type::node);
        REQUIRE(it->ref() == 123);
        REQUIRE(std::string{it->role()}.empty());
        ++it;
        REQUIRE(it == rml.end());
    }

    SECTION("Valid format: three members") {
        const char* const s = "n123@,w456@abc,r78@type";
        oid::opl_parse_relation_members(s, s + std::strlen(s), buffer);
        REQUIRE(buffer.written() > 0);
        const auto& rml = buffer.get<osmium::RelationMemberList>(0);
        REQUIRE(rml.size() == 3);
        auto it = rml.begin();
        REQUIRE(it->type() == osmium::item_type::node);
        REQUIRE(it->ref() == 123);
        REQUIRE(std::string{it->role()}.empty());
        ++it;
        REQUIRE(it->type() == osmium::item_type::way);
        REQUIRE(it->ref() == 456);
        REQUIRE(std::string{it->role()} == "abc");
        ++it;
        REQUIRE(it->type() == osmium::item_type::relation);
        REQUIRE(it->ref() == 78);
        REQUIRE(std::string{it->role()} == "type");
        ++it;
        REQUIRE(it == rml.end());
    }

    SECTION("Trailing comma") {
        const char* const s = "n123@,w456@abc,r78@type,";
        oid::opl_parse_relation_members(s, s + std::strlen(s), buffer);
        REQUIRE(buffer.written() > 0);
        const auto& rml = buffer.get<osmium::RelationMemberList>(0);
        REQUIRE(rml.size() == 3);
        auto it = rml.begin();
        REQUIRE(it->type() == osmium::item_type::node);
        REQUIRE(it->ref() == 123);
        REQUIRE(std::string{it->role()}.empty());
        ++it;
        REQUIRE(it->type() == osmium::item_type::way);
        REQUIRE(it->ref() == 456);
        REQUIRE(std::string{it->role()} == "abc");
        ++it;
        REQUIRE(it->type() == osmium::item_type::relation);
        REQUIRE(it->ref() == 78);
        REQUIRE(std::string{it->role()} == "type");
        ++it;
        REQUIRE(it == rml.end());
    }


}

TEST_CASE("Parse node") {
    osmium::memory::Buffer buffer{1024};

    SECTION("Node with id only") {
        const char* s = "17";
        const char* e = s + std::strlen(s);
        oid::opl_parse_node(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Node& node = buffer.get<osmium::Node>(0);
        REQUIRE(node.id() == 17);
    }

    SECTION("Node with trailing space") {
        const char* s = "17 ";
        const char* e = s + std::strlen(s);
        oid::opl_parse_node(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Node& node = buffer.get<osmium::Node>(0);
        REQUIRE(node.id() == 17);
    }

    SECTION("Node with id and version") {
        const char* s = "17 v23";
        const char* e = s + std::strlen(s);
        oid::opl_parse_node(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Node& node = buffer.get<osmium::Node>(0);
        REQUIRE(node.id() == 17);
        REQUIRE(node.version() == 23);
    }

    SECTION("Node with multiple spaces") {
        const char* s = "17  v23";
        const char* e = s + std::strlen(s);
        oid::opl_parse_node(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Node& node = buffer.get<osmium::Node>(0);
        REQUIRE(node.id() == 17);
        REQUIRE(node.version() == 23);
    }

    SECTION("Node with tab instead of space") {
        const char* s = "17\tv23";
        const char* e = s + std::strlen(s);
        oid::opl_parse_node(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Node& node = buffer.get<osmium::Node>(0);
        REQUIRE(node.id() == 17);
        REQUIRE(node.version() == 23);
    }

    SECTION("Full node (no tags)") {
        const char* s = "125799 v6 dV c7711393 t2011-03-29T21:43:10Z i45445 uUScha T x8.7868047 y53.0749415";
        const char* e = s + std::strlen(s);
        oid::opl_parse_node(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Node& node = buffer.get<osmium::Node>(0);
        REQUIRE(node.id() == 125799);
        REQUIRE(node.version() == 6);
        REQUIRE(node.visible());
        REQUIRE(node.changeset() == 7711393);
        REQUIRE(node.timestamp() == osmium::Timestamp{"2011-03-29T21:43:10Z"});
        REQUIRE(node.uid() == 45445);
        REQUIRE(std::string{node.user()} == "UScha");
        osmium::Location loc{8.7868047, 53.0749415};
        REQUIRE(node.location() == loc);
        REQUIRE(node.tags().empty());
    }

    SECTION("Node with tags)") {
        const char* s = "123 v1 c456 Thighway=residential,oneway=true,name=High%20%Street";
        const char* e = s + std::strlen(s);
        oid::opl_parse_node(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Node& node = buffer.get<osmium::Node>(0);
        REQUIRE(node.id() == 123);
        REQUIRE(node.version() == 1);
        REQUIRE(node.changeset() == 456);
        REQUIRE(node.tags().size() == 3);

        auto it = node.tags().cbegin();
        REQUIRE(std::string{it->key()} == "highway");
        REQUIRE(std::string{it->value()} == "residential");
        ++it;
        REQUIRE(std::string{it->key()} == "oneway");
        REQUIRE(std::string{it->value()} == "true");
        ++it;
        REQUIRE(std::string{it->key()} == "name");
        REQUIRE(std::string{it->value()} == "High Street");
        ++it;
        REQUIRE(it == node.tags().cend());
    }

    SECTION("Order does not matter") {
        const char* s = "125799 c7711393 dV v6 i45445 uUScha T t2011-03-29T21:43:10Z y53.0749415 x8.7868047";
        const char* e = s + std::strlen(s);
        oid::opl_parse_node(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Node& node = buffer.get<osmium::Node>(0);
        REQUIRE(node.id() == 125799);
        REQUIRE(node.version() == 6);
        REQUIRE(node.visible());
        REQUIRE(node.changeset() == 7711393);
        REQUIRE(node.timestamp() == osmium::Timestamp{"2011-03-29T21:43:10Z"});
        REQUIRE(node.uid() == 45445);
        REQUIRE(std::string{node.user()} == "UScha");
        osmium::Location loc{8.7868047, 53.0749415};
        REQUIRE(node.location() == loc);
        REQUIRE(node.tags().empty());
    }
}

TEST_CASE("Parse way") {

    osmium::memory::Buffer buffer{1024};

    SECTION("Way with id only") {
        const char* s = "17";
        const char* e = s + std::strlen(s);
        oid::opl_parse_way(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Way& way = buffer.get<osmium::Way>(0);
        REQUIRE(way.id() == 17);
    }

    SECTION("Complete way") {
        const char* s = "78216 v12 dV c35895909 t2015-12-11T22:01:57Z i7412 umjulius Tdestination=Interlaken;%20%Kandersteg;%20%Zweisimmen,highway=motorway_link,name=Thun%20%Süd,oneway=yes,ref=17,surface=asphalt Nn1011242,n2569390773,n2569390769,n255308687,n2569390761,n255308689,n255308691,n1407526499,n255308692,n3888362655,n255308693,n255308694,n255308695,n255308686";
        const char* e = s + std::strlen(s);
        oid::opl_parse_way(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Way& way = buffer.get<osmium::Way>(0);
        REQUIRE(way.id() == 78216);
        REQUIRE(way.version() == 12);
        REQUIRE(way.visible());
        REQUIRE(way.changeset() == 35895909);
        REQUIRE(way.timestamp() == osmium::Timestamp{"2015-12-11T22:01:57Z"});
        REQUIRE(way.uid() == 7412);
        REQUIRE(std::string{way.user()} == "mjulius");
        REQUIRE(way.tags().size() == 6);

        auto it = way.tags().cbegin();
        REQUIRE(std::string{it->key()} == "destination");
        REQUIRE(std::string{it->value()} == "Interlaken; Kandersteg; Zweisimmen");
        ++it;
        REQUIRE(std::string{it->key()} == "highway");
        REQUIRE(std::string{it->value()} == "motorway_link");
        ++it;
        REQUIRE(std::string{it->key()} == "name");
        REQUIRE(std::string{it->value()} == "Thun Süd");
        ++it;
        REQUIRE(std::string{it->key()} == "oneway");
        REQUIRE(std::string{it->value()} == "yes");
        ++it;
        REQUIRE(std::string{it->key()} == "ref");
        REQUIRE(std::string{it->value()} == "17");
        ++it;
        REQUIRE(std::string{it->key()} == "surface");
        REQUIRE(std::string{it->value()} == "asphalt");
        ++it;
        REQUIRE(it == way.tags().cend());

        REQUIRE(way.nodes().size() == 14);
        std::vector<osmium::object_id_type> ids = {
            1011242, 2569390773, 2569390769, 255308687, 2569390761, 255308689,
            255308691, 1407526499, 255308692, 3888362655, 255308693, 255308694,
            255308695, 255308686
        };
        REQUIRE(std::equal(way.nodes().cbegin(), way.nodes().cend(), ids.cbegin()));
    }

}

TEST_CASE("Parse relation") {

    osmium::memory::Buffer buffer{1024};

    SECTION("Relation with id only") {
        const char* s = "17";
        const char* e = s + std::strlen(s);
        oid::opl_parse_relation(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Relation& relation = buffer.get<osmium::Relation>(0);
        REQUIRE(relation.id() == 17);
    }

    SECTION("Complete relation") {
        const char* s = "1074 v45 dV c20048094 t2014-01-17T10:27:04Z i86566 uwisieb Ttype=multipolygon,landuse=forest Mw255722275@inner,w256126142@outer,w24402792@inner,w256950103@outer,w255722279@outer";
        const char* e = s + std::strlen(s);
        oid::opl_parse_relation(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Relation& relation = buffer.get<osmium::Relation>(0);
        REQUIRE(relation.id() == 1074);
        REQUIRE(relation.version() == 45);
        REQUIRE(relation.visible());
        REQUIRE(relation.changeset() == 20048094);
        REQUIRE(relation.timestamp() == osmium::Timestamp{"2014-01-17T10:27:04Z"});
        REQUIRE(relation.uid() == 86566);
        REQUIRE(std::string{relation.user()} == "wisieb");
        REQUIRE(relation.tags().size() == 2);

        auto it = relation.tags().cbegin();
        REQUIRE(std::string{it->key()} == "type");
        REQUIRE(std::string{it->value()} == "multipolygon");
        ++it;
        REQUIRE(std::string{it->key()} == "landuse");
        REQUIRE(std::string{it->value()} == "forest");
        ++it;
        REQUIRE(it == relation.tags().cend());

        REQUIRE(relation.members().size() == 5);
        auto mit = relation.members().cbegin();
        REQUIRE(mit->type() == osmium::item_type::way);
        REQUIRE(mit->ref() == 255722275);
        REQUIRE(std::string{mit->role()} == "inner");
        ++mit;
        REQUIRE(mit->type() == osmium::item_type::way);
        REQUIRE(mit->ref() == 256126142);
        REQUIRE(std::string{mit->role()} == "outer");
        ++mit;
        ++mit;
        ++mit;
        ++mit;
        REQUIRE(mit == relation.members().cend());
    }

}

TEST_CASE("Parse changeset") {

    osmium::memory::Buffer buffer{1024};

    SECTION("Changeset with id only") {
        const char* s = "17";
        const char* e = s + std::strlen(s);
        oid::opl_parse_changeset(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Changeset& changeset = buffer.get<osmium::Changeset>(0);
        REQUIRE(changeset.id() == 17);
    }

    SECTION("Complete changeset") {
        const char* s = "873494 k1 s2009-04-21T08:52:49Z e2009-04-21T09:52:49Z d0 i13093 uTiberiusNero x13.923302 y50.957069 X14.0337519 Y50.9824084 Tcreated_by=Potlatch%20%0.11";
        const char* e = s + std::strlen(s);
        oid::opl_parse_changeset(&s, buffer);
        REQUIRE(s == e);
        REQUIRE(buffer.written() > 0);
        const osmium::Changeset& changeset = buffer.get<osmium::Changeset>(0);
        REQUIRE(changeset.id() == 873494);
        REQUIRE(changeset.created_at() == osmium::Timestamp{"2009-04-21T08:52:49Z"});
        REQUIRE(changeset.closed_at() == osmium::Timestamp{"2009-04-21T09:52:49Z"});
        REQUIRE(changeset.num_changes() == 1);
        REQUIRE(changeset.num_comments() == 0);
        REQUIRE(changeset.uid() == 13093);
        REQUIRE(std::string{changeset.user()} == "TiberiusNero");
        REQUIRE(changeset.tags().size() == 1);

        auto it = changeset.tags().cbegin();
        REQUIRE(std::string{it->key()} == "created_by");
        REQUIRE(std::string{it->value()} == "Potlatch 0.11");
        ++it;
        REQUIRE(it == changeset.tags().cend());

        osmium::Box box{13.923302, 50.957069, 14.0337519, 50.9824084};
        REQUIRE(box == changeset.bounds());
    }

}

TEST_CASE("Parse line") {

    osmium::memory::Buffer buffer{1024};

    SECTION("Empty line") {
        const char* s = "";
        REQUIRE_FALSE(oid::opl_parse_line(0, "", buffer));
        REQUIRE(buffer.written() == 0);
    }

    SECTION("Comment line") {
        REQUIRE_FALSE(oid::opl_parse_line(0, "# abc", buffer));
        REQUIRE(buffer.written() == 0);
    }

    SECTION("Fail") {
        REQUIRE_THROWS_WITH(oid::opl_parse_line(0, "X", buffer),
                            "OPL error: unknown type on line 0 column 0");
        REQUIRE(buffer.written() == 0);
    }

    SECTION("New line at end not allowed") {
        REQUIRE_THROWS_WITH(oid::opl_parse_line(0, "n12 v3\n", buffer),
                            "OPL error: expected space or tab character on line 0 column 6");
    }

    SECTION("Node, but not asking for nodes") {
        REQUIRE_FALSE(oid::opl_parse_line(0, "n12 v1", buffer, osmium::osm_entity_bits::way));
        REQUIRE(buffer.written() == 0);
    }

    SECTION("Node") {
        REQUIRE(oid::opl_parse_line(0, "n12 v3", buffer));
        REQUIRE(buffer.written() > 0);
        const auto& item = buffer.get<osmium::memory::Item>(0);
        REQUIRE(item.type() == osmium::item_type::node);
    }

    SECTION("Way") {
        REQUIRE(oid::opl_parse_line(0, "w12 v3", buffer));
        REQUIRE(buffer.written() > 0);
        const auto& item = buffer.get<osmium::memory::Item>(0);
        REQUIRE(item.type() == osmium::item_type::way);
    }

    SECTION("Relation") {
        REQUIRE(oid::opl_parse_line(0, "r12 v3", buffer));
        REQUIRE(buffer.written() > 0);
        const auto& item = buffer.get<osmium::memory::Item>(0);
        REQUIRE(item.type() == osmium::item_type::relation);
    }

    SECTION("Changeset") {
        REQUIRE(oid::opl_parse_line(0, "c12", buffer));
        REQUIRE(buffer.written() > 0);
        const auto& item = buffer.get<osmium::memory::Item>(0);
        REQUIRE(item.type() == osmium::item_type::changeset);
    }

}

TEST_CASE("Get context for errors") {

    osmium::memory::Buffer buffer{1024};

    SECTION("Unknown object type") {
        bool error = false;
        try {
            oid::opl_parse_line(0, "~~~", buffer);
        } catch (const osmium::opl_error& e) {
            error = true;
            REQUIRE(e.line == 0);
            REQUIRE(e.column == 0);
            REQUIRE(std::string{e.data} == "~~~");
        }
        REQUIRE(error);
    }

    SECTION("Node id") {
        bool error = false;
        try {
            oid::opl_parse_line(0, "n~~~", buffer);
        } catch (const osmium::opl_error& e) {
            error = true;
            REQUIRE(e.line == 0);
            REQUIRE(e.column == 1);
            REQUIRE(std::string{e.data} == "~~~");
        }
        REQUIRE(error);
    }

    SECTION("Node expect space") {
        bool error = false;
        try {
            oid::opl_parse_line(1, "n123~~~", buffer);
        } catch (const osmium::opl_error& e) {
            error = true;
            REQUIRE(e.line == 1);
            REQUIRE(e.column == 4);
            REQUIRE(std::string{e.data} == "~~~");
        }
        REQUIRE(error);
    }

    SECTION("Node unknown attribute") {
        bool error = false;
        try {
            oid::opl_parse_line(2, "n123 ~~~", buffer);
        } catch (const osmium::opl_error& e) {
            error = true;
            REQUIRE(e.line == 2);
            REQUIRE(e.column == 5);
            REQUIRE(std::string{e.data} == "~~~");
        }
        REQUIRE(error);
    }

    SECTION("Node version not an int") {
        bool error = false;
        try {
            oid::opl_parse_line(3, "n123 v~~~", buffer);
        } catch (const osmium::opl_error& e) {
            error = true;
            REQUIRE(e.line == 3);
            REQUIRE(e.column == 6);
            REQUIRE(std::string{e.data} == "~~~");
        }
        REQUIRE(error);
    }

}

TEST_CASE("Parse line with external interface") {

    osmium::memory::Buffer buffer{1024};

    SECTION("Node") {
        REQUIRE(osmium::opl_parse("n12 v3", buffer));
        REQUIRE(buffer.committed() > 0);
        REQUIRE(buffer.written() == buffer.committed());
        const auto& item = buffer.get<osmium::memory::Item>(0);
        REQUIRE(item.type() == osmium::item_type::node);
        REQUIRE(static_cast<const osmium::Node&>(item).id() == 12);
    }

    SECTION("Empty line") {
        REQUIRE_FALSE(osmium::opl_parse("", buffer));
        REQUIRE(buffer.written() == 0);
        REQUIRE(buffer.committed() == 0);
    }

    SECTION("Failure") {
        REQUIRE_THROWS_WITH(osmium::opl_parse("x", buffer),
                            "OPL error: unknown type on line 0 column 0");
        REQUIRE(buffer.written() == 0);
        REQUIRE(buffer.committed() == 0);
    }

}

TEST_CASE("Duplicate attributes") {
    osmium::memory::Buffer buffer{1024};
    REQUIRE_THROWS_WITH(osmium::opl_parse("n123 v1 v2", buffer),
                        "OPL error: Duplicate attribute: version (v) on line 0 column 0");
    REQUIRE_THROWS_WITH(osmium::opl_parse("w123 c1 c2", buffer),
                        "OPL error: Duplicate attribute: changeset_id (c) on line 0 column 0");
    REQUIRE_THROWS_WITH(osmium::opl_parse("r123 Ta=b Tc=d", buffer),
                        "OPL error: Duplicate attribute: tags (T) on line 0 column 0");
    REQUIRE_THROWS_WITH(osmium::opl_parse("c123 k1 k2", buffer),
                        "OPL error: Duplicate attribute: num_changes (k) on line 0 column 0");

    for (const char *attr : {"v1", "dV", "c2", "t2020-01-01T00:00:01Z", "i3", "utest", "Ta=b", "x1.0", "y2.0"}) {
        auto line = std::string{"n1 "} + attr;
        REQUIRE_NOTHROW(osmium::opl_parse(line.c_str(), buffer));
        line += " ";
        line += attr;
        REQUIRE_THROWS_AS(osmium::opl_parse(line.c_str(), buffer),
                          const osmium::opl_error &);
    }

    for (const char *attr : {"v1", "dV", "c2", "t2020-01-01T00:00:01Z", "i3", "utest", "Ta=b", "Nn1"}) {
        auto line = std::string{"w1 "} + attr;
        REQUIRE_NOTHROW(osmium::opl_parse(line.c_str(), buffer));
        line += " ";
        line += attr;
        REQUIRE_THROWS_AS(osmium::opl_parse(line.c_str(), buffer),
                          const osmium::opl_error &);
    }

    for (const char *attr : {"v1", "dV", "c2", "t2020-01-01T00:00:01Z", "i3", "utest", "Ta=b", "Mn1@foo"}) {
        auto line = std::string{"r1 "} + attr;
        REQUIRE_NOTHROW(osmium::opl_parse(line.c_str(), buffer));
        line += " ";
        line += attr;
        REQUIRE_THROWS_AS(osmium::opl_parse(line.c_str(), buffer),
                          const osmium::opl_error &);
    }

    for (const char *attr : {"k1", "s2020-01-01T00:00:01Z", "e2020-01-01T00:00:02Z", "d1", "i3", "utest", "Ta=b", "x1", "y2", "X3", "Y4"}) {
        auto line = std::string{"c1 "} + attr;
        REQUIRE_NOTHROW(osmium::opl_parse(line.c_str(), buffer));
        line += " ";
        line += attr;
        REQUIRE_THROWS_AS(osmium::opl_parse(line.c_str(), buffer),
                          const osmium::opl_error &);
    }
}

TEST_CASE("Parse OPL using Reader") {
    osmium::io::File file{with_data_dir("t/io/data.opl")};
    osmium::io::Reader reader{file};

    const auto buffer = reader.read();
    REQUIRE(buffer);
    const auto& node = buffer.get<osmium::Node>(0);
    REQUIRE(node.id() == 1);
}

TEST_CASE("Parse OPL with CRLF line ending using Reader") {
    osmium::io::File file{with_data_dir("t/io/data-cr.opl")};
    osmium::io::Reader reader{file};

    const auto buffer = reader.read();
    REQUIRE(buffer);
    const auto& node = buffer.get<osmium::Node>(0);
    REQUIRE(node.id() == 1);
}

TEST_CASE("Parse OPL with missing newline using Reader") {
    osmium::io::File file{with_data_dir("t/io/data-nonl.opl")};
    osmium::io::Reader reader{file};

    const auto buffer = reader.read();
    REQUIRE(buffer);
    const auto& node = buffer.get<osmium::Node>(0);
    REQUIRE(node.id() == 1);
}

class lbl_tester {

    std::vector<std::string> m_inputs;
    std::vector<std::string> m_outputs;

public:

    lbl_tester(const std::initializer_list<std::string>& inputs,
               const std::initializer_list<std::string>& outputs) :
        m_inputs(inputs),
        m_outputs(outputs) {
    }

    bool input_done() {
        return m_inputs.empty();
    }

    std::string get_input() {
        REQUIRE_FALSE(m_inputs.empty());
        std::string data = std::move(m_inputs.front());
        m_inputs.erase(m_inputs.begin());
        return data;
    }

    void parse_line(const char *data) {
        REQUIRE_FALSE(m_outputs.empty());
        REQUIRE(m_outputs.front() == data);
        m_outputs.erase(m_outputs.begin());
    }

    void check() {
        REQUIRE(m_inputs.empty());
        REQUIRE(m_outputs.empty());
    }

}; // class lbl_tester

void check_lbl(const std::initializer_list<std::string>& in,
               const std::initializer_list<std::string>& out) {
    lbl_tester tester{in, out};
    osmium::io::detail::line_by_line(tester);
    tester.check();
}

TEST_CASE("line_by_line for OPL parser 1") {
    check_lbl({""}, {});
}

TEST_CASE("line_by_line for OPL parser 2") {
    check_lbl({"\n"}, {});
}

TEST_CASE("line_by_line for OPL parser 3") {
    check_lbl({"foo\n"}, {"foo"});
}

TEST_CASE("line_by_line for OPL parser 4") {
    check_lbl({"foo"}, {"foo"});
}

TEST_CASE("line_by_line for OPL parser 5") {
    check_lbl({"foo\nbar\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 6") {
    check_lbl({"foo\nbar"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 7") {
    check_lbl({"foo\nbar\nbaz\n"}, {"foo", "bar", "baz"});
}

TEST_CASE("line_by_line for OPL parser 8") {
    check_lbl({"foo\n", "bar\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 9") {
    check_lbl({"foo\nb", "ar\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 10") {
    check_lbl({"foo\nb", "ar\n", "baz\n"}, {"foo", "bar", "baz"});
}

TEST_CASE("line_by_line for OPL parser 11") {
    check_lbl({"foo", "\nbar\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 12") {
    check_lbl({"foo", "\nbar"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 13") {
    check_lbl({"foo", "\nbar", "\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 14") {
    check_lbl({"foo\n", "b", "ar\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 15") {
    check_lbl({"foo\n", "ba", "r\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 16") {
    check_lbl({"foo", "\n", "bar\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 17") {
    check_lbl({"foo\r\nbar\r\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 18") {
    check_lbl({"foo\r\nb", "ar\r\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 19") {
    check_lbl({"foo\r\nb", "ar\n", "baz\r\n"}, {"foo", "bar", "baz"});
}

TEST_CASE("line_by_line for OPL parser 20") {
    check_lbl({"foo", "\r\nbar\r\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 21") {
    check_lbl({"foo\r", "\nbar"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 22") {
    check_lbl({"foo", "\r\nbar\r", "\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 23") {
    check_lbl({"foo\r\n", "b", "ar\r\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 24") {
    check_lbl({"foo\n", "ba", "r\r\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 25") {
    check_lbl({"foo", "\n", "bar\r\n"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 26") {
    check_lbl({"foo", "\n\r", "bar\r"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 27") {
    check_lbl({"foo\r", "bar\n\r"}, {"foo", "bar"});
}

TEST_CASE("line_by_line for OPL parser 28") {
    check_lbl({"foo\nb", "ar"}, {"foo", "bar"});
}

