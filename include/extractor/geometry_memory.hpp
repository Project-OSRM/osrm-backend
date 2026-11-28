#ifndef OSRM_EXTRACTOR_GEOMETRY_MEMORY_HPP
#define OSRM_EXTRACTOR_GEOMETRY_MEMORY_HPP

#include <cstddef>
#include <memory_resource>
#include <new>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#endif

namespace osrm::extractor
{
class GeometryMemory final : public std::pmr::memory_resource
{
#if defined(__APPLE__)
    malloc_zone_t *m_zone = nullptr;
#endif

    void *do_allocate(std::size_t bytes, std::size_t alignment) override
    {
#if defined(__APPLE__)
        if (!m_zone)
            m_zone = malloc_create_zone(0, 0);
        if (!m_zone)
            throw std::bad_alloc();
        void *result = alignment <= alignof(std::max_align_t)
                           ? malloc_zone_malloc(m_zone, bytes)
                           : malloc_zone_memalign(m_zone, alignment, bytes);
        if (!result)
            throw std::bad_alloc();
        return result;
#else
        return std::pmr::new_delete_resource()->allocate(bytes, alignment);
#endif
    }

    void do_deallocate(void *address,
                       [[maybe_unused]] std::size_t bytes,
                       [[maybe_unused]] std::size_t alignment) override
    {
#if defined(__APPLE__)
        malloc_zone_free(m_zone, address);
#else
        std::pmr::new_delete_resource()->deallocate(address, bytes, alignment);
#endif
    }

    bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override
    { return this == &other; }

  public:
    ~GeometryMemory() override { release(); }

    void release() noexcept
    {
#if defined(__APPLE__)
        if (m_zone)
        {
            malloc_destroy_zone(m_zone);
            m_zone = nullptr;
        }
#endif
    }
};
} // namespace osrm::extractor

#endif
