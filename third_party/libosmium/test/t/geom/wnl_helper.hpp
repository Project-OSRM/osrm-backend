
#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/node_ref_list.hpp>

using namespace osmium::builder::attr;

inline const osmium::WayNodeList& create_test_wnl_okay(osmium::memory::Buffer& buffer) {
    const auto pos = osmium::builder::add_way_node_list(buffer, _nodes({
        {1, {3.2, 4.2}},
        {3, {3.5, 4.7}},
        {4, {3.5, 4.7}},
        {2, {3.6, 4.9}}
    }));

    return buffer.get<osmium::WayNodeList>(pos);
}

inline const osmium::WayNodeList& create_test_wnl_closed(osmium::memory::Buffer& buffer) {
    const auto pos = osmium::builder::add_way_node_list(buffer, _nodes({
        {1, {3.0, 3.0}},
        {2, {4.1, 4.1}},
        {3, {4.1, 4.1}},
        {4, {3.6, 4.1}},
        {5, {3.1, 3.5}},
        {6, {3.0, 3.0}},
    }));

    return buffer.get<osmium::WayNodeList>(pos);
}

inline const osmium::WayNodeList& create_test_wnl_empty(osmium::memory::Buffer& buffer) {
    {
        osmium::builder::WayNodeListBuilder wnl_builder(buffer);
    }

    return buffer.get<osmium::WayNodeList>(buffer.commit());
}

inline const osmium::WayNodeList& create_test_wnl_same_location(osmium::memory::Buffer& buffer) {
    const auto pos = osmium::builder::add_way_node_list(buffer, _nodes({
        {1, {3.5, 4.7}},
        {2, {3.5, 4.7}}
    }));

    return buffer.get<osmium::WayNodeList>(pos);
}

inline const osmium::WayNodeList& create_test_wnl_undefined_location(osmium::memory::Buffer& buffer) {
    const auto pos = osmium::builder::add_way_node_list(buffer, _nodes({
        {1, {3.5, 4.7}},
        {2, osmium::Location()}
    }));

    return buffer.get<osmium::WayNodeList>(pos);
}

