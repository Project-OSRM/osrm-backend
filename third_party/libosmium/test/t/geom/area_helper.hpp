
#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/area.hpp>

using namespace osmium::builder::attr;

inline const osmium::Area& create_test_area_1outer_0inner(osmium::memory::Buffer& buffer) {
    osmium::builder::add_area(buffer,
        _user("foo"),
        _tag("building", "true"),
        _outer_ring({
            {1, {3.2, 4.2}},
            {2, {3.5, 4.7}},
            {3, {3.6, 4.9}},
            {1, {3.2, 4.2}}
        })
    );

    return buffer.get<osmium::Area>(0);
}

inline const osmium::Area& create_test_area_1outer_1inner(osmium::memory::Buffer& buffer) {
    osmium::builder::add_area(buffer,
        _user("foo"),
        _tag("building", "true"),
        _outer_ring({
            {1, {0.1, 0.1}},
            {2, {9.1, 0.1}},
            {3, {9.1, 9.1}},
            {4, {0.1, 9.1}},
            {1, {0.1, 0.1}}
        }),
        _inner_ring({
            {5, {1.0, 1.0}},
            {6, {8.0, 1.0}},
            {7, {8.0, 8.0}},
            {8, {1.0, 8.0}},
            {5, {1.0, 1.0}}
        })
    );

    return buffer.get<osmium::Area>(0);
}

inline const osmium::Area& create_test_area_2outer_2inner(osmium::memory::Buffer& buffer) {
    osmium::builder::add_area(buffer,
        _user("foo"),
        _tag("building", "true"),
        _outer_ring({
            {1, {0.1, 0.1}},
            {2, {9.1, 0.1}},
            {3, {9.1, 9.1}},
            {4, {0.1, 9.1}},
            {1, {0.1, 0.1}}
        }),
        _inner_ring({
            {5, {1.0, 1.0}},
            {6, {4.0, 1.0}},
            {7, {4.0, 4.0}},
            {8, {1.0, 4.0}},
            {5, {1.0, 1.0}}
        }),
        _inner_ring({
            {10, {5.0, 5.0}},
            {11, {5.0, 7.0}},
            {12, {7.0, 7.0}},
            {10, {5.0, 5.0}}
        }),
        _outer_ring({
            {100, {10.0, 10.0}},
            {101, {11.0, 10.0}},
            {102, {11.0, 11.0}},
            {103, {10.0, 11.0}},
            {100, {10.0, 10.0}}
        })
    );

    return buffer.get<osmium::Area>(0);
}

