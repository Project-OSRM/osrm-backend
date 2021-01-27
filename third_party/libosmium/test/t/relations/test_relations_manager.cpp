#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/xml_input.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/relations/relations_manager.hpp>

#include <iterator>

struct EmptyRM : public osmium::relations::RelationsManager<EmptyRM, true, true, true> {
};

struct TestRM : public osmium::relations::RelationsManager<TestRM, true, true, true> {

    std::size_t count_new_rels      = 0;
    std::size_t count_new_members   = 0;
    std::size_t count_complete_rels = 0;
    std::size_t count_before        = 0;
    std::size_t count_not_in_any    = 0;
    std::size_t count_after         = 0;

    bool new_relation(const osmium::Relation& /*relation*/) noexcept {
        ++count_new_rels;
        return true;
    }

    bool new_member(const osmium::Relation& /*relation*/, const osmium::RelationMember& /*member*/, std::size_t /*n*/) noexcept {
        ++count_new_members;
        return true;
    }

    void complete_relation(const osmium::Relation& /*relation*/) noexcept {
        ++count_complete_rels;
    }

    void before_node(const osmium::Node& /*node*/) noexcept {
        ++count_before;
    }

    void node_not_in_any_relation(const osmium::Node& /*node*/) noexcept {
        ++count_not_in_any;
    }

    void after_node(const osmium::Node& /*node*/) noexcept {
        ++count_after;
    }

    void before_way(const osmium::Way& /*way*/) noexcept {
        ++count_before;
    }

    void way_not_in_any_relation(const osmium::Way& /*way*/) noexcept {
        ++count_not_in_any;
    }

    void after_way(const osmium::Way& /*way*/) noexcept {
        ++count_after;
    }

    void before_relation(const osmium::Relation& /*relation*/) noexcept {
        ++count_before;
    }

    void relation_not_in_any_relation(const osmium::Relation& /*relation*/) noexcept {
        ++count_not_in_any;
    }

    void after_relation(const osmium::Relation& /*relation*/) noexcept {
        ++count_after;
    }

};

struct CallbackRM : public osmium::relations::RelationsManager<CallbackRM, true, false, false> {

    std::size_t count_nodes = 0;

    bool new_relation(const osmium::Relation& /*relation*/) noexcept {
        return true;
    }

    bool new_member(const osmium::Relation& /*relation*/, const osmium::RelationMember& member, std::size_t /*n*/) noexcept {
        return member.type() == osmium::item_type::node;
    }

    void complete_relation(const osmium::Relation& relation) {
        for (const auto& member : relation.members()) {
            if (member.type() == osmium::item_type::node) {
                ++count_nodes;
                const auto* node = get_member_node(member.ref());
                REQUIRE(node);
                buffer().add_item(*node);
                buffer().commit();
            }
        }
    }

};

struct AnyRM : public osmium::relations::RelationsManager<AnyRM, true, true, true> {
    bool new_relation(const osmium::Relation& /*relation*/) noexcept {
        return true;
    }

    bool new_member(const osmium::Relation& /*relation*/, const osmium::RelationMember& /*member*/, std::size_t /*n*/) noexcept {
        return true;
    }
};

TEST_CASE("Use RelationsManager without any overloaded functions in derived class") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};

    EmptyRM manager;

    osmium::relations::read_relations(file, manager);

    REQUIRE(manager.member_nodes_database().size()     == 2);
    REQUIRE(manager.member_ways_database().size()      == 2);
    REQUIRE(manager.member_relations_database().size() == 1);

    REQUIRE(manager.member_database(osmium::item_type::node).size()     == 2);
    REQUIRE(manager.member_database(osmium::item_type::way).size()      == 2);
    REQUIRE(manager.member_database(osmium::item_type::relation).size() == 1);

    const auto& m = manager;
    REQUIRE(m.member_database(osmium::item_type::node).size()     == 2);
    REQUIRE(m.member_database(osmium::item_type::way).size()      == 2);
    REQUIRE(m.member_database(osmium::item_type::relation).size() == 1);

    osmium::io::Reader reader{file};
    osmium::apply(reader, manager.handler());
    reader.close();
}

TEST_CASE("Relations manager derived class") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};

    TestRM manager;

    osmium::relations::read_relations(file, manager);

    REQUIRE(manager.member_nodes_database().size()     == 2);
    REQUIRE(manager.member_ways_database().size()      == 2);
    REQUIRE(manager.member_relations_database().size() == 1);

    bool callback_called = false;
    osmium::io::Reader reader{file};
    osmium::apply(reader, manager.handler([&](osmium::memory::Buffer&& /*unused*/) {
        callback_called = true;
    }));
    reader.close();
    REQUIRE_FALSE(callback_called);

    REQUIRE(manager.count_new_rels      ==  3);
    REQUIRE(manager.count_new_members   ==  5);
    REQUIRE(manager.count_complete_rels ==  2);
    REQUIRE(manager.count_before        == 10);
    REQUIRE(manager.count_not_in_any    ==  6);
    REQUIRE(manager.count_after         == 10);

    int n = 0;
    manager.for_each_incomplete_relation([&](const osmium::relations::RelationHandle& handle){
        ++n;
        REQUIRE(handle->id() == 31);
        for (const auto& member : handle->members()) {
            const auto* obj = manager.get_member_object(member);
            if (member.ref() == 22) {
                REQUIRE_FALSE(obj);
            } else {
                REQUIRE(obj);
            }
        }
    });
    REQUIRE(n == 1);
}

TEST_CASE("Relations manager with callback") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};

    CallbackRM manager;

    osmium::relations::read_relations(file, manager);

    REQUIRE(manager.member_nodes_database().size()     == 2);
    REQUIRE(manager.member_ways_database().size()      == 0);
    REQUIRE(manager.member_relations_database().size() == 0);

    bool callback_called = false;
    osmium::io::Reader reader{file};
    osmium::apply(reader, manager.handler([&](osmium::memory::Buffer&& buffer) {
        callback_called = true;
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 2);
    }));
    reader.close();
    REQUIRE(manager.count_nodes == 2);
    REQUIRE(callback_called);
}

TEST_CASE("Relations manager reading buffer without callback") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};

    CallbackRM manager;

    osmium::relations::read_relations(file, manager);

    REQUIRE(manager.member_nodes_database().size()     == 2);
    REQUIRE(manager.member_ways_database().size()      == 0);
    REQUIRE(manager.member_relations_database().size() == 0);

    osmium::io::Reader reader{file};
    osmium::apply(reader, manager.handler());
    reader.close();

    auto buffer = manager.read();
    REQUIRE(std::distance(buffer.begin(), buffer.end()) == 2);

    REQUIRE(manager.count_nodes == 2);
}

TEST_CASE("Access members via RelationsManager") {
    EmptyRM manager;

    manager.prepare_for_lookup();

    REQUIRE(nullptr == manager.get_member_node(0));
    REQUIRE(nullptr == manager.get_member_way(0));
    REQUIRE(nullptr == manager.get_member_relation(0));

    REQUIRE(nullptr == manager.get_member_node(17));
    REQUIRE(nullptr == manager.get_member_way(17));
    REQUIRE(nullptr == manager.get_member_relation(17));
}

TEST_CASE("Handle duplicate members correctly") {
    osmium::io::File file{with_data_dir("t/relations/dupl_member.osm")};

    TestRM manager;

    osmium::relations::read_relations(file, manager);

    auto c = manager.member_nodes_database().count();
    REQUIRE(c.tracked   == 5);
    REQUIRE(c.available == 0);
    REQUIRE(c.removed   == 0);

    osmium::io::Reader reader{file};
    osmium::apply(reader, manager.handler());
    reader.close();

    c = manager.member_nodes_database().count();
    REQUIRE(c.tracked   == 0);
    REQUIRE(c.available == 0);
    REQUIRE(c.removed   == 5);

    REQUIRE(manager.count_new_rels      == 2);
    REQUIRE(manager.count_new_members   == 5);
    REQUIRE(manager.count_complete_rels == 2);
    REQUIRE(manager.count_not_in_any    == 2); // 2 relations
}

TEST_CASE("Check handling of missing members") {
    osmium::io::File file{with_data_dir("t/relations/missing_members.osm")};

    AnyRM manager;

    osmium::relations::read_relations(file, manager);

    osmium::io::Reader reader{file};
    osmium::apply(reader, manager.handler());
    reader.close();


    size_t nodes = 0;
    size_t ways = 0;
    size_t relations = 0;
    size_t missing_nodes = 0;
    size_t missing_ways = 0;
    size_t missing_relations = 0;

    manager.for_each_incomplete_relation([&](const osmium::relations::RelationHandle& handle){
        if (handle->id() != 31) {
            // count relation 31 only
            return;
        }
        for (const auto& member : handle->members()) {
            // RelationMember::ref() is supposed to returns 0 if we are interested in the member.
            // RelationsManagerBase::get_member_object() is supposed to return a nullptr if the
            // member is not available (missing in the input file).
            const osmium::OSMObject* object = manager.get_member_object(member);
            switch (member.type()) {
            case osmium::item_type::node :
                ++nodes;
                if (member.ref() != 0 && !object) {
                    ++missing_nodes;
                }
                break;
            case osmium::item_type::way :
                ++ways;
                if (member.ref() != 0 && !object) {
                    ++missing_ways;
                }
                break;
            case osmium::item_type::relation :
                ++relations;
                if (member.ref() != 0 && !object) {
                    ++missing_relations;
                }
                break;
            default:
                break;
            }
        }
    });
    REQUIRE(nodes == 2);
    REQUIRE(ways == 3);
    REQUIRE(relations == 3);
    REQUIRE(missing_nodes == 1);
    REQUIRE(missing_ways == 1);
    REQUIRE(missing_relations == 2);
}

