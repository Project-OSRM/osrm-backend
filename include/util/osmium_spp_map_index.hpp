#ifndef SPP_MEM_MAP_HPP
#define SPP_MEM_MAP_HPP

#include <algorithm> // IWYU pragma: keep (for std::copy)
#include <cstddef>
#include <iterator>
#include <map>
#include <vector>

#include <osmium/index/index.hpp>
#include <osmium/index/map.hpp>
#include <osmium/io/detail/read_write.hpp>

#include <sparsepp/spp.h>

namespace osrm
{
namespace util
{

/**
 * This implementation uses std::map internally. It uses rather a
 * lot of memory, but might make sense for small maps.
 */
template <typename TId, typename TValue>
class SparsePPMemMap : public osmium::index::map::Map<TId, TValue>
{

    // This is a rough estimate for the memory needed for each
    // element in the map (id + value + pointers to left, right,
    // and parent plus some overhead for color of red-black-tree
    // or similar).
    static constexpr size_t element_size = sizeof(TId) + sizeof(TValue) + sizeof(void *) * 4;

    spp::sparse_hash_map<TId, TValue> m_elements;

  public:
    SparsePPMemMap() = default;

    ~SparsePPMemMap() noexcept final = default;

    void set(const TId id, const TValue value) final { m_elements[id] = value; }

    TValue get(const TId id) const final
    {
        const auto it = m_elements.find(id);
        if (it == m_elements.end())
        {
            throw osmium::not_found{id};
        }
        return it->second;
    }

    TValue get_noexcept(const TId id) const noexcept final
    {
        const auto it = m_elements.find(id);
        if (it == m_elements.end())
        {
            return osmium::index::empty_value<TValue>();
        }
        return it->second;
    }

    size_t size() const noexcept final { return m_elements.size(); }

    size_t used_memory() const noexcept final { return element_size * m_elements.size(); }

    void clear() final { m_elements.clear(); }

    void dump_as_list(const int fd) final
    {
        using t = typename spp::sparse_hash_map<TId, TValue>::value_type;
        std::vector<t> v;
        v.reserve(m_elements.size());
        std::copy(m_elements.cbegin(), m_elements.cend(), std::back_inserter(v));
        osmium::io::detail::reliable_write(
            fd, reinterpret_cast<const char *>(v.data()), sizeof(t) * v.size());
    }

}; // class SparsePPMemMap

} // namespace util

} // namespace osrm

#endif // OSMIUM_INDEX_MAP_SPARSE_MEM_MAP_HPP
