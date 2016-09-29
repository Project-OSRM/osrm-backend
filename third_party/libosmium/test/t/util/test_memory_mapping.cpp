#include "catch.hpp"

#include <sys/types.h>
#include <limits>

#include <osmium/util/file.hpp>
#include <osmium/util/memory_mapping.hpp>

#if defined(_MSC_VER) || (defined(__GNUC__) && defined(_WIN32))
#include "win_mkstemp.hpp"
#endif

static const size_t huge = std::numeric_limits<size_t>::max();

TEST_CASE("anonymous mapping") {

    SECTION("simple memory mapping should work") {
        osmium::util::MemoryMapping mapping(1000, osmium::util::MemoryMapping::mapping_mode::write_private);
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

    SECTION("memory mapping of zero length should fail") {
        REQUIRE_THROWS({
             osmium::util::MemoryMapping mapping(0, osmium::util::MemoryMapping::mapping_mode::write_private);
        });
    }

    SECTION("moving a memory mapping should work") {
        osmium::util::MemoryMapping mapping1(1000, osmium::util::MemoryMapping::mapping_mode::write_private);
        int* addr1 = mapping1.get_addr<int>();
        *addr1 = 42;

        REQUIRE(!!mapping1);
        osmium::util::MemoryMapping mapping2(std::move(mapping1));
        REQUIRE(!!mapping2);
        REQUIRE(!mapping1);
        mapping1.unmap();

        int* addr2 = mapping2.get_addr<int>();
        REQUIRE(*addr2 == 42);

        mapping2.unmap();
        REQUIRE(!mapping2);
    }

    SECTION("move assignment should work") {
        osmium::util::MemoryMapping mapping1(1000, osmium::util::MemoryMapping::mapping_mode::write_private);
        osmium::util::MemoryMapping mapping2(1000, osmium::util::MemoryMapping::mapping_mode::write_private);

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
    SECTION("remapping to larger size should work") {
        osmium::util::MemoryMapping mapping(1000, osmium::util::MemoryMapping::mapping_mode::write_private);
        REQUIRE(mapping.size() >= 1000);

        size_t size1 = mapping.size();

        int* addr1 = mapping.get_addr<int>();
        *addr1 = 42;

        mapping.resize(8000);
        REQUIRE(mapping.size() > size1);

        int* addr2 = mapping.get_addr<int>();
        REQUIRE(*addr2 == 42);
    }

    SECTION("remapping to smaller size should work") {
        osmium::util::MemoryMapping mapping(8000, osmium::util::MemoryMapping::mapping_mode::write_private);
        REQUIRE(mapping.size() >= 1000);

        size_t size1 = mapping.size();

        int* addr1 = mapping.get_addr<int>();
        *addr1 = 42;

        mapping.resize(500);
        REQUIRE(mapping.size() < size1);

        int* addr2 = mapping.get_addr<int>();
        REQUIRE(*addr2 == 42);
    }
#endif

}

TEST_CASE("file-based mapping") {

    SECTION("writing to a mapped file should work") {
        char filename[] = "test_mmap_write_XXXXXX";
        const int fd = mkstemp(filename);
        REQUIRE(fd > 0);

        osmium::util::resize_file(fd, 100);

        {
            osmium::util::MemoryMapping mapping(100, osmium::util::MemoryMapping::mapping_mode::write_shared, fd);
            REQUIRE(mapping.writable());

            REQUIRE(!!mapping);
            REQUIRE(mapping.size() >= 100);

            *mapping.get_addr<int>() = 1234;

            mapping.unmap();
        }

        REQUIRE(osmium::util::file_size(fd) == 100);

        {
            osmium::util::MemoryMapping mapping(100, osmium::util::MemoryMapping::mapping_mode::readonly, fd);
            REQUIRE(!mapping.writable());

            REQUIRE(!!mapping);
            REQUIRE(mapping.size() >= 100);
            REQUIRE(*mapping.get_addr<int>() == 1234);

            mapping.unmap();
        }

        REQUIRE(0 == close(fd));
        REQUIRE(0 == unlink(filename));
    }

    SECTION("writing to a privately mapped file should work") {
        char filename[] = "test_mmap_write_XXXXXX";
        const int fd = mkstemp(filename);
        REQUIRE(fd > 0);

        osmium::util::resize_file(fd, 100);

        {
            osmium::util::MemoryMapping mapping(100, osmium::util::MemoryMapping::mapping_mode::write_private, fd);
            REQUIRE(mapping.writable());

            REQUIRE(!!mapping);
            REQUIRE(mapping.size() >= 100);

            *mapping.get_addr<int>() = 1234;

            mapping.unmap();
        }

        REQUIRE(osmium::util::file_size(fd) == 100);

        {
            osmium::util::MemoryMapping mapping(100, osmium::util::MemoryMapping::mapping_mode::readonly, fd);
            REQUIRE(!mapping.writable());

            REQUIRE(!!mapping);
            REQUIRE(mapping.size() >= 100);
            REQUIRE(*mapping.get_addr<int>() == 0); // should not see the value set above

            mapping.unmap();
        }

        REQUIRE(0 == close(fd));
        REQUIRE(0 == unlink(filename));
    }

    SECTION("remapping to larger size should work") {
        char filename[] = "test_mmap_grow_XXXXXX";
        const int fd = mkstemp(filename);
        REQUIRE(fd > 0);

        osmium::util::MemoryMapping mapping(100, osmium::util::MemoryMapping::mapping_mode::write_shared, fd);
        REQUIRE(mapping.size() >= 100);
        size_t size1 = mapping.size();

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

    SECTION("remapping to smaller size should work") {
        char filename[] = "test_mmap_shrink_XXXXXX";
        const int fd = mkstemp(filename);
        REQUIRE(fd > 0);

        {
            osmium::util::MemoryMapping mapping(8000, osmium::util::MemoryMapping::mapping_mode::write_shared, fd);
            REQUIRE(mapping.size() >= 8000);
            size_t size1 = mapping.size();

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
}

TEST_CASE("typed anonymous mapping") {

    SECTION("simple memory mapping should work") {
        osmium::util::TypedMemoryMapping<uint32_t> mapping(1000);
        volatile uint32_t* addr = mapping.begin();

        REQUIRE(mapping.writable());

        *addr = 42;
        REQUIRE(*addr == 42);

        REQUIRE(!!mapping);
        mapping.unmap();
        REQUIRE(!mapping);
        mapping.unmap(); // second unmap is okay
    }

    SECTION("moving a memory mapping should work") {
        osmium::util::TypedMemoryMapping<uint32_t> mapping1(1000);
        uint32_t* addr1 = mapping1.begin();
        *addr1 = 42;

        REQUIRE(!!mapping1);
        osmium::util::TypedMemoryMapping<uint32_t> mapping2(std::move(mapping1));
        REQUIRE(!!mapping2);
        REQUIRE(!mapping1);
        mapping1.unmap();

        auto addr2 = mapping2.begin();
        REQUIRE(*addr2 == 42);

        mapping2.unmap();
        REQUIRE(!mapping2);
    }

    SECTION("move assignment should work") {
        osmium::util::TypedMemoryMapping<uint32_t> mapping1(1000);
        osmium::util::TypedMemoryMapping<uint32_t> mapping2(1000);

        REQUIRE(!!mapping1);
        REQUIRE(!!mapping2);

        auto addr1 = mapping1.begin();
        *addr1 = 42;

        mapping2 = std::move(mapping1);
        REQUIRE(!!mapping2);
        REQUIRE(!mapping1);

        auto addr2 = mapping2.begin();
        REQUIRE(*addr2 == 42);

        mapping2.unmap();
        REQUIRE(!mapping2);
    }

#ifdef __linux__
    SECTION("remapping to larger size should work") {
        osmium::util::TypedMemoryMapping<uint32_t> mapping(1000);
        REQUIRE(mapping.size() >= 1000);

        auto addr1 = mapping.begin();
        *addr1 = 42;

        mapping.resize(8000);

        auto addr2 = mapping.begin();
        REQUIRE(*addr2 == 42);
    }

    SECTION("remapping to smaller size should work") {
        osmium::util::TypedMemoryMapping<uint32_t> mapping(8000);
        REQUIRE(mapping.size() >= 8000);

        auto addr1 = mapping.begin();
        *addr1 = 42;

        mapping.resize(500);

        auto addr2 = mapping.begin();
        REQUIRE(*addr2 == 42);
    }
#endif

}

TEST_CASE("typed file-based mapping") {

    SECTION("writing to a mapped file should work") {
        char filename[] = "test_mmap_file_size_XXXXXX";
        const int fd = mkstemp(filename);
        REQUIRE(fd > 0);

        osmium::util::resize_file(fd, 100);

        {
            osmium::util::TypedMemoryMapping<uint32_t> mapping(100, osmium::util::MemoryMapping::mapping_mode::write_shared, fd);
            REQUIRE(mapping.writable());

            REQUIRE(!!mapping);
            REQUIRE(mapping.size() >= 100);

            *mapping.begin() = 1234;

            mapping.unmap();
        }

        {
            osmium::util::TypedMemoryMapping<uint32_t> mapping(100, osmium::util::MemoryMapping::mapping_mode::readonly, fd);
            REQUIRE(!mapping.writable());

            REQUIRE(!!mapping);
            REQUIRE(mapping.size() >= 100);
            REQUIRE(*mapping.begin() == 1234);

            mapping.unmap();
        }

        REQUIRE(0 == close(fd));
        REQUIRE(0 == unlink(filename));
    }

}

TEST_CASE("anonymous memory mapping class") {

    SECTION("simple memory mapping should work") {
        osmium::util::AnonymousMemoryMapping mapping(1000);
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
    SECTION("remapping to larger size should work") {
        osmium::util::AnonymousMemoryMapping mapping(1000);
        REQUIRE(mapping.size() >= 1000);

        int* addr1 = mapping.get_addr<int>();
        *addr1 = 42;

        mapping.resize(2000);

        int* addr2 = mapping.get_addr<int>();
        REQUIRE(*addr2 == 42);
    }

    SECTION("remapping to smaller size should work") {
        osmium::util::AnonymousMemoryMapping mapping(2000);
        REQUIRE(mapping.size() >= 2000);

        int* addr1 = mapping.get_addr<int>();
        *addr1 = 42;

        mapping.resize(500);

        int* addr2 = mapping.get_addr<int>();
        REQUIRE(*addr2 == 42);
    }
#endif

}

