#ifndef OSRM_STOARGE_VIEW_FACTORY_HPP
#define OSRM_STOARGE_VIEW_FACTORY_HPP

#include "storage/shared_datatype.hpp"

#include "util/vector_view.hpp"

namespace osrm
{
namespace storage
{

template <typename T>
util::vector_view<T> make_vector_view(char *memory_ptr, DataLayout layout, const std::string &name)
{
    return util::vector_view<T>(layout.GetBlockPtr<T>(memory_ptr, name),
                                layout.GetBlockEntries(name));
}

template <>
inline util::vector_view<bool>
make_vector_view(char *memory_ptr, DataLayout layout, const std::string &name)
{
    return util::vector_view<bool>(
        layout.GetBlockPtr<util::vector_view<bool>::Word>(memory_ptr, name),
        layout.GetBlockEntries(name));
}
}
}

#endif
