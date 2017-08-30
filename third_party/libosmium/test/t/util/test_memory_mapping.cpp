#include "catch.hpp"

#include <cstdlib>
#include <limits>
#include <utility>

#include <osmium/util/file.hpp>
#include <osmium/util/memory_mapping.hpp>

#if defined(_MSC_VER) || (defined(__GNUC__) && defined(_WIN32))
#include "win_mkstemp.hpp"
#endif

static const size_t huge = std::numeric_limits<size_t>::max();

TEST_CASE("Anonymous mapping: simple memory mapping should work") {
    osmium::util::MemoryMapping mapping{1000, osmium::util::MemoryMapping::mapping_mode::write_private};
    REQUIRE(mapping.get_addr() != nullptr);

    REQUIRE(mapping.size() >= 1000);

    volatile int* addr = mapping.get_addr<int>();

    REQUIRE(mapping.writable());

    *addr = 42;
    REQUIRE(*addr == 42);

    REQUIRE(!!mapping);
    mapping.unmap();
    REQUIRE(!mapping);
    mapping.unmap(); // second unmap is okay
}

TEST_CASE("Anonymous mapping: memory mapping of zero length should result in memory mapping of pagesize length") {
    osmium::util::MemoryMapping mapping{0, osmium::util::MemoryMapping::mapping_mode::write_private};
    REQUIRE(mapping.size() == osmium::util::get_pagesize());
}

TEST_CASE("Anonymous mapping: moving a memory mapping should work") {
    osmium::util::MemoryMapping mapping1{1000, osmium::util::MemoryMapping::mapping_mode::write_private};
    int* addr1 = mapping1.get_addr<int>();
    *addr1 = 42;

    REQUIRE(!!mapping1);
    osmium::util::MemoryMapping mapping2{std::move(mapping1)};
    REQUIRE(!!mapping2);
    REQUIRE(!mapping1);
    mapping1.unmap();

    int* addr2 = mapping2.get_addr<int>();
    REQUIRE(*addr2 == 42);

    mapping2.unmap();
    REQUIRE(!mapping2);
}

TEST_CASE("Anonymous mapping: move assignment should work") {
    osmium::util::MemoryMapping mapping1{1000, osmium::util::MemoryMapping::mapping_mode::write_private};
    osmium::util::MemoryMapping mapping2{1000, osmium::util::MemoryMapping::mapping_mode::write_private};

    REQUIRE(!!mapping1);
    REQUIRE(!!mapping2);

    int* addr1 = mapping1.get_addr<int>();
    *addr1 = 42;

    mapping2 = std::move(mapping1);
    REQUIRE(!!mapping2);
    REQUIRE(!mapping1);

    int* addr2 = mapping2.get_addr<int>();
    REQUIRE(*addr2 == 42);

    mapping2.unmap();
    REQUIRE(!mapping2);
}

#ifdef __linux__
TEST_CASE("Anonymous mapping: remapping to larger size should work") {
    osmium::util::MemoryMapping mapping{1000, osmium::util::MemoryMapping::mapping_mode::write_private};
    REQUIRE(mapping.size() >= 1000);

    const size_t size1 = mapping.size();

    int* addr1 = mapping.get_addr<int>();
    *addr1 = 42;

    mapping.resize(8000);
    REQUIRE(mapping.size() > size1);

    int* addr2 = mapping.get_addr<int>();
    REQUIRE(*addr2 == 42);
}

TEST_CASE("Anonymous mapping: remapping to smaller size should work") {
    osmium::util::MemoryMapping mapping{8000, osmium::util::MemoryMapping::mapping_mode::write_private};
    REQUIRE(mapping.size() >= 1000);

    const size_t size1 = mapping.size();

    int* addr1 = mapping.get_addr<int>();
    *addr1 = 42;

    mapping.resize(500);
    REQUIRE(mapping.size() < size1);

    int* addr2 = mapping.get_addr<int>();
    REQUIRE(*addr2 == 42);
}
#endif

TEST_CASE("File-based mapping: writing to a mapped file should work") {
    char filename[] = "test_mmap_write_XXXXXX";
    const int fd = mkstemp(filename);
    REQUIRE(fd > 0);

    osmium::util::resize_file(fd, 100);

    {
        osmium::util::MemoryMapping mapping{100, osmium::util::MemoryMapping::mapping_mode::write_shared, fd};
        REQUIRE(mapping.writable());

        REQUIRE(!!mapping);
        REQUIRE(mapping.size() >= 100);

        *mapping.get_addr<int>() = 1234;

        mapping.unmap();
    }

    REQUIRE(osmium::util::file_size(fd) == 100);

    {
        osmium::util::MemoryMapping mapping{100, osmium::util::MemoryMapping::mapping_mode::readonly, fd};
        REQUIRE(!mapping.writable());

        REQUIRE(!!mapping);
        REQUIRE(mapping.size() >= 100);
        REQUIRE(*mapping.get_addr<int>() == 1234);

        mapping.unmap();
    }

    REQUIRE(0 == close(fd));
    REQUIRE(0 == unlink(filename));
}

TEST_CASE("File-based mapping: Reading from a zero-sized mapped file should work") {
    char filename[] = "test_mmap_read_zero_XXXXXX";
    const int fd = mkstemp(filename);
    REQUIRE(fd > 0);

    {
        osmium::util::MemoryMapping mapping{0, osmium::util::MemoryMapping::mapping_mode::readonly, fd};
        REQUIRE(mapping.size() > 0);
        mapping.unmap();
    }

    REQUIRE(0 == close(fd));
    REQUIRE(0 == unlink(filename));
}

TEST_CASE("File-based mapping: writing to a privately mapped file should work") {
    char filename[] = "test_mmap_write_XXXXXX";
    const int fd = mkstemp(filename);
    REQUIRE(fd > 0);

    osmium::util::resize_file(fd, 100);

    {
        osmium::util::MemoryMapping mapping{100, osmium::util::MemoryMapping::mapping_mode::write_private, fd};
        REQUIRE(mapping.writable());

        REQUIRE(!!mapping);
        REQUIRE(mapping.size() >= 100);

        *mapping.get_addr<int>() = 1234;

        mapping.unmap();
    }

    REQUIRE(osmium::util::file_size(fd) == 100);

    {
        osmium::util::MemoryMapping mapping{100, osmium::util::MemoryMapping::mapping_mode::readonly, fd};
        REQUIRE(!mapping.writable());

        REQUIRE(!!mapping);
        REQUIRE(mapping.size() >= 100);
        REQUIRE(*mapping.get_addr<int>() == 0); // should not see the value set above

        mapping.unmap();
    }

    REQUIRE(0 == close(fd));
    REQUIRE(0 == unlink(filename));
}

TEST_CASE("File-based mapping: remapping to larger size should work") {
    char filename[] = "test_mmap_grow_XXXXXX";
    const int fd = mkstemp(filename);
    REQUIRE(fd > 0);

    osmium::util::MemoryMapping mapping{100, osmium::util::MemoryMapping::mapping_mode::write_shared, fd};
    REQUIRE(mapping.size() >= 100);
    const size_t size1 = mapping.size();

    int* addr1 = mapping.get_addr<int>();
    *addr1 = 42;

    mapping.resize(8000);
    REQUIRE(mapping.size() >= 8000);
    REQUIRE(mapping.size() > size1);

    int* addr2 = mapping.get_addr<int>();
    REQUIRE(*addr2 == 42);

    mapping.unmap();

    REQUIRE(0 == close(fd));
    REQUIRE(0 == unlink(filename));
}

TEST_CASE("File-based mapping: remapping to smaller size should work") {
    char filename[] = "test_mmap_shrink_XXXXXX";
    const int fd = mkstemp(filename);
    REQUIRE(fd > 0);

    {
        osmium::util::MemoryMapping mapping{8000, osmium::util::MemoryMapping::mapping_mode::write_shared, fd};
        REQUIRE(mapping.size() >= 8000);
        const size_t size1 = mapping.size();

        int* addr1 = mapping.get_addr<int>();
        *addr1 = 42;

        mapping.resize(50);
        REQUIRE(mapping.size() >= 50);
        REQUIRE(mapping.size() < size1);

        int* addr2 = mapping.get_addr<int>();
        REQUIRE(*addr2 == 42);
    }

    REQUIRE(0 == close(fd));
    REQUIRE(0 == unlink(filename));
}

TEST_CASE("Typed anonymous mapping: simple memory mapping should work") {
    osmium::util::TypedMemoryMapping<uint32_t> mapping{1000};
    volatile uint32_t* addr = mapping.begin();

    REQUIRE(mapping.writable());

    *addr = 42;
    REQUIRE(*addr == 42);

    REQUIRE(!!mapping);
    mapping.unmap();
    REQUIRE(!mapping);
    mapping.unmap(); // second unmap is okay
}

TEST_CASE("Typed anonymous mapping: moving a memory mapping should work") {
    osmium::util::TypedMemoryMapping<uint32_t> mapping1{1000};
    uint32_t* addr1 = mapping1.begin();
    *addr1 = 42;

    REQUIRE(!!mapping1);
    osmium::util::TypedMemoryMapping<uint32_t> mapping2{std::move(mapping1)};
    REQUIRE(!!mapping2);
    REQUIRE(!mapping1);
    mapping1.unmap();

    const auto addr2 = mapping2.begin();
    REQUIRE(*addr2 == 42);

    mapping2.unmap();
    REQUIRE(!mapping2);
}

TEST_CASE("Typed anonymous mapping: move assignment should work") {
    osmium::util::TypedMemoryMapping<uint32_t> mapping1{1000};
    osmium::util::TypedMemoryMapping<uint32_t> mapping2{1000};

    REQUIRE(!!mapping1);
    REQUIRE(!!mapping2);

    auto addr1 = mapping1.begin();
    *addr1 = 42;

    mapping2 = std::move(mapping1);
    REQUIRE(!!mapping2);
    REQUIRE(!mapping1);

    const auto addr2 = mapping2.begin();
    REQUIRE(*addr2 == 42);

    mapping2.unmap();
    REQUIRE(!mapping2);
}

#ifdef __linux__
TEST_CASE("Typed anonymous mapping: remapping to larger size should work") {
    osmium::util::TypedMemoryMapping<uint32_t> mapping{1000};
    REQUIRE(mapping.size() >= 1000);

    auto addr1 = mapping.begin();
    *addr1 = 42;

    mapping.resize(8000);

    const auto addr2 = mapping.begin();
    REQUIRE(*addr2 == 42);
}

TEST_CASE("Typed anonymous mapping: remapping to smaller size should work") {
    osmium::util::TypedMemoryMapping<uint32_t> mapping{8000};
    REQUIRE(mapping.size() >= 8000);

    auto addr1 = mapping.begin();
    *addr1 = 42;

    mapping.resize(500);

    const auto addr2 = mapping.begin();
    REQUIRE(*addr2 == 42);
}
#endif

TEST_CASE("Typed file-based mapping: writing to a mapped file should work") {
    char filename[] = "test_mmap_file_size_XXXXXX";
    const int fd = mkstemp(filename);
    REQUIRE(fd > 0);

    osmium::util::resize_file(fd, 100);

    {
        osmium::util::TypedMemoryMapping<uint32_t> mapping{100, osmium::util::MemoryMapping::mapping_mode::write_shared, fd};
        REQUIRE(mapping.writable());

        REQUIRE(!!mapping);
        REQUIRE(mapping.size() >= 100);

        *mapping.begin() = 1234;

        mapping.unmap();
    }

    {
        osmium::util::TypedMemoryMapping<uint32_t> mapping{100, osmium::util::MemoryMapping::mapping_mode::readonly, fd};
        REQUIRE(!mapping.writable());

        REQUIRE(!!mapping);
        REQUIRE(mapping.size() >= 100);
        REQUIRE(*mapping.begin() == 1234);

        mapping.unmap();
    }

    REQUIRE(0 == close(fd));
    REQUIRE(0 == unlink(filename));
}

TEST_CASE("Anonymous memory mapping class: simple memory mapping should work") {
    osmium::util::AnonymousMemoryMapping mapping{1000};
    REQUIRE(mapping.get_addr() != nullptr);

    volatile int* addr = mapping.get_addr<int>();

    REQUIRE(mapping.writable());

    *addr = 42;
    REQUIRE(*addr == 42);

    REQUIRE(!!mapping);
    mapping.unmap();
    REQUIRE(!mapping);
    mapping.unmap(); // second unmap is okay
}

#ifdef __linux__
TEST_CASE("Anonymous memory mapping class: remapping to larger size should work") {
    osmium::util::AnonymousMemoryMapping mapping{1000};
    REQUIRE(mapping.size() >= 1000);

    int* addr1 = mapping.get_addr<int>();
    *addr1 = 42;

    mapping.resize(2000);

    const int* addr2 = mapping.get_addr<int>();
    REQUIRE(*addr2 == 42);
}

TEST_CASE("Anonymous memory mapping class: remapping to smaller size should work") {
    osmium::util::AnonymousMemoryMapping mapping{2000};
    REQUIRE(mapping.size() >= 2000);

    int* addr1 = mapping.get_addr<int>();
    *addr1 = 42;

    mapping.resize(500);

    const int* addr2 = mapping.get_addr<int>();
    REQUIRE(*addr2 == 42);
}
#endif

