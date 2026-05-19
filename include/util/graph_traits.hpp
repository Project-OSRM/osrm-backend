#ifndef OSRM_GRAPH_TRAITS_HPP
#define OSRM_GRAPH_TRAITS_HPP

#include <concepts>

namespace osrm::util::traits
{

template <typename T>
concept HasDataMember = requires(T t) { t.data; };

template <typename T>
concept HasTargetMember = requires(T t) { t.target; };

template <typename T>
concept HasDataAndTargetMember = HasDataMember<T> && HasTargetMember<T>;

template <typename T>
concept HasFirstEdgeMember = requires(T t) { t.first_edge; };

} // namespace osrm::util::traits

#endif // OSRM_GRAPH_TRAITS_HPP
