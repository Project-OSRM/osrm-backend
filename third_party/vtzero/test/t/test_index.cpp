
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/index.hpp>

#include <map>
#include <string>
#include <unordered_map>

TEST_CASE("add keys to layer using key index built into layer") {
    static constexpr const int max_keys = 100;

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    for (int n = 0; n < max_keys; ++n) {
        const auto key = std::to_string(n);
        const auto idx = lbuilder.add_key(key);
        REQUIRE(n == idx.value());
    }

    for (int n = 0; n < max_keys; n += 2) {
        const auto key = std::to_string(n);
        const auto idx = lbuilder.add_key(key);
        REQUIRE(n == idx.value());
    }
}

TEST_CASE("add values to layer using value index built into layer") {
    static constexpr const int max_values = 100;

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    for (int n = 0; n < max_values; ++n) {
        const auto value = std::to_string(n);
        const auto idx = lbuilder.add_value(vtzero::encoded_property_value{value});
        REQUIRE(n == idx.value());
    }

    for (int n = 0; n < max_values; n += 2) {
        const auto value = std::to_string(n);
        const auto idx = lbuilder.add_value(vtzero::encoded_property_value{value});
        REQUIRE(n == idx.value());
    }
}

template <typename TIndex>
static void test_key_index() {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    TIndex index{lbuilder};

    const auto i1 = index({"foo"});
    const auto i2 = index({"bar"});
    const auto i3 = index({"baz"});
    const auto i4 = index({"foo"});
    const auto i5 = index({"foo"});
    const auto i6 = index({""});
    const auto i7 = index({"bar"});

    REQUIRE(i1 != i2);
    REQUIRE(i1 != i3);
    REQUIRE(i1 == i4);
    REQUIRE(i1 == i5);
    REQUIRE(i1 != i6);
    REQUIRE(i1 != i7);
    REQUIRE(i2 != i3);
    REQUIRE(i2 != i4);
    REQUIRE(i2 != i5);
    REQUIRE(i2 != i6);
    REQUIRE(i2 == i7);
    REQUIRE(i3 != i4);
    REQUIRE(i3 != i5);
    REQUIRE(i3 != i6);
    REQUIRE(i3 != i7);
    REQUIRE(i4 == i5);
    REQUIRE(i4 != i6);
    REQUIRE(i4 != i7);
    REQUIRE(i5 != i6);
    REQUIRE(i5 != i7);
    REQUIRE(i6 != i7);
}

TEST_CASE("key index based on std::unordered_map") {
    test_key_index<vtzero::key_index<std::unordered_map>>();
}

TEST_CASE("key index based on std::map") {
    test_key_index<vtzero::key_index<std::map>>();
}

template <typename TIndex>
static void test_value_index_internal() {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    TIndex index{lbuilder};

    const auto i1 = index(vtzero::encoded_property_value{"foo"});
    const auto i2 = index(vtzero::encoded_property_value{"bar"});
    const auto i3 = index(vtzero::encoded_property_value{88});
    const auto i4 = index(vtzero::encoded_property_value{"foo"});
    const auto i5 = index(vtzero::encoded_property_value{77});
    const auto i6 = index(vtzero::encoded_property_value{1.5});
    const auto i7 = index(vtzero::encoded_property_value{"bar"});

    REQUIRE(i1 != i2);
    REQUIRE(i1 != i3);
    REQUIRE(i1 == i4);
    REQUIRE(i1 != i5);
    REQUIRE(i1 != i6);
    REQUIRE(i1 != i7);

    REQUIRE(i2 != i3);
    REQUIRE(i2 != i4);
    REQUIRE(i2 != i5);
    REQUIRE(i2 != i6);
    REQUIRE(i2 == i7);

    REQUIRE(i3 != i4);
    REQUIRE(i3 != i5);
    REQUIRE(i3 != i6);
    REQUIRE(i3 != i7);

    REQUIRE(i4 != i5);
    REQUIRE(i4 != i6);
    REQUIRE(i4 != i7);

    REQUIRE(i5 != i6);
    REQUIRE(i5 != i7);

    REQUIRE(i6 != i7);
}

TEST_CASE("internal value index based on std::unordered_map") {
    test_value_index_internal<vtzero::value_index_internal<std::unordered_map>>();
}

TEST_CASE("internal value index based on std::map") {
    test_value_index_internal<vtzero::value_index_internal<std::map>>();
}

TEST_CASE("external value index") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    vtzero::value_index<vtzero::string_value_type, std::string, std::map> string_index{lbuilder};
    vtzero::value_index<vtzero::int_value_type, int, std::unordered_map> int_index{lbuilder};
    vtzero::value_index<vtzero::sint_value_type, int, std::unordered_map> sint_index{lbuilder};

    const auto i1 = string_index("foo");
    const auto i2 = string_index("bar");
    const auto i3 = int_index(6);
    const auto i4 = sint_index(6);
    const auto i5 = string_index(std::string{"foo"});
    const auto i6 = int_index(6);
    const auto i7 = sint_index(2);
    const auto i8 = sint_index(5);
    const auto i9 = sint_index(6);

    REQUIRE(i1 != i2);
    REQUIRE(i1 != i3);
    REQUIRE(i1 != i4);
    REQUIRE(i1 == i5);
    REQUIRE(i1 != i6);
    REQUIRE(i1 != i7);
    REQUIRE(i1 != i7);
    REQUIRE(i1 != i9);

    REQUIRE(i2 != i3);
    REQUIRE(i2 != i4);
    REQUIRE(i2 != i5);
    REQUIRE(i2 != i6);
    REQUIRE(i2 != i7);
    REQUIRE(i2 != i8);
    REQUIRE(i2 != i9);

    REQUIRE(i3 != i4);
    REQUIRE(i3 != i5);
    REQUIRE(i3 == i6);
    REQUIRE(i3 != i7);
    REQUIRE(i3 != i8);
    REQUIRE(i3 != i9);

    REQUIRE(i4 != i5);
    REQUIRE(i4 != i6);
    REQUIRE(i4 != i7);
    REQUIRE(i4 != i8);
    REQUIRE(i4 == i9);

    REQUIRE(i5 != i6);
    REQUIRE(i5 != i7);
    REQUIRE(i5 != i8);
    REQUIRE(i5 != i9);

    REQUIRE(i6 != i7);
    REQUIRE(i6 != i8);
    REQUIRE(i6 != i9);

    REQUIRE(i7 != i8);
    REQUIRE(i7 != i9);

    REQUIRE(i8 != i9);
}

TEST_CASE("bool value index") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::value_index_bool index{lbuilder};

    const auto i1 = index(false);
    const auto i2 = index(true);
    const auto i3 = index(true);
    const auto i4 = index(false);

    REQUIRE(i1 != i2);
    REQUIRE(i1 != i3);
    REQUIRE(i1 == i4);

    REQUIRE(i2 == i3);
    REQUIRE(i2 != i4);

    REQUIRE(i3 != i4);
}

TEST_CASE("small unsigned int value index") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::value_index_small_uint index{lbuilder};

    const auto i1 = index(12);
    const auto i2 = index(4);
    const auto i3 = index(0);
    const auto i4 = index(100);
    const auto i5 = index(4);
    const auto i6 = index(12);

    REQUIRE(i1 != i2);
    REQUIRE(i1 != i3);
    REQUIRE(i1 != i4);
    REQUIRE(i1 != i5);
    REQUIRE(i1 == i6);

    REQUIRE(i2 != i3);
    REQUIRE(i2 != i4);
    REQUIRE(i2 == i5);
    REQUIRE(i2 != i6);

    REQUIRE(i3 != i4);
    REQUIRE(i3 != i5);
    REQUIRE(i3 != i6);

    REQUIRE(i4 != i5);
    REQUIRE(i4 != i6);

    REQUIRE(i5 != i6);
}

TEST_CASE("add features using a key index") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    vtzero::point_feature_builder fbuilder{lbuilder};
    fbuilder.set_id(7);
    fbuilder.add_point(10, 20);

    SECTION("no index") {
        fbuilder.add_property("some_key", 12);
    }

    SECTION("key index using unordered_map") {
        vtzero::key_index<std::unordered_map> index{lbuilder};
        fbuilder.add_property(index("some_key"), 12);
    }

    SECTION("key index using map") {
        vtzero::key_index<std::map> index{lbuilder};
        fbuilder.add_property(index("some_key"), 12);
    }

    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    // ============

    vtzero::vector_tile tile{data};

    REQUIRE(tile.count_layers() == 1);
    auto layer = tile.next_layer();
    REQUIRE(layer.num_features() == 1);
    auto feature = layer.next_feature();
    REQUIRE(feature.id() == 7);
    const auto property = feature.next_property();
    REQUIRE(property.value().int_value() == 12);
}

TEST_CASE("add features using a value index") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    const auto key = lbuilder.add_key("some_key");

    vtzero::point_feature_builder fbuilder{lbuilder};
    fbuilder.set_id(17);
    fbuilder.add_point(10, 20);

    SECTION("no index") {
        fbuilder.add_property(key, vtzero::sint_value_type{12});
    }

    SECTION("external value index using unordered_map") {
        vtzero::value_index<vtzero::sint_value_type, int, std::unordered_map> index{lbuilder};
        fbuilder.add_property(key, index(12));
    }

    SECTION("external value index using map") {
        vtzero::value_index<vtzero::sint_value_type, int, std::map> index{lbuilder};
        fbuilder.add_property(key, index(12));
    }

    SECTION("property_value_type index") {
        vtzero::value_index_internal<std::unordered_map> index{lbuilder};
        fbuilder.add_property(key, index(vtzero::encoded_property_value{vtzero::sint_value_type{12}}));
    }

    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    // ============

    vtzero::vector_tile tile{data};

    REQUIRE(tile.count_layers() == 1);
    auto layer = tile.next_layer();
    REQUIRE(layer.num_features() == 1);
    auto feature = layer.next_feature();
    REQUIRE(feature.id() == 17);
    const auto property = feature.next_property();
    REQUIRE(property.value().sint_value() == 12);
}

