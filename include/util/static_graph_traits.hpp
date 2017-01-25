#ifndef STATIC_GRAPH_TRAITS_HPP
#define STATIC_GRAPH_TRAITS_HPP

#include <type_traits>

namespace osrm
{
namespace util
{

namespace traits
{

// Introspection for an arbitrary .data member attribute
template <typename T, typename = void> struct HasDataMember : std::false_type
{
};

template <typename T>
struct HasDataMember<T, decltype((void)(sizeof(std::declval<T>().data) > 0))> : std::true_type
{
};

// Introspection for an arbitrary .target member attribute
template <typename T, typename = void> struct HasTargetMember : std::false_type
{
};

template <typename T>
struct HasTargetMember<T, decltype((void)(sizeof(std::declval<T>().target) > 0))> : std::true_type
{
};

// Static Graph requires edges to have a .target and .data member attribute
template <typename Edge>
struct HasDataAndTargetMember
    : std::integral_constant<bool, HasDataMember<Edge>() && HasTargetMember<Edge>()>
{
};

// Static Graph requires nodes to have a .first_edge member attribute
template <typename T, typename = void> struct HasFirstEdgeMember : std::false_type
{
};

template <typename T>
struct HasFirstEdgeMember<T, decltype((void)(sizeof(std::declval<T>().first_edge) > 0))>
    : std::true_type
{
};

} // ns traits
} // ns util
} // ns osrm

#endif // STATIC_GRAPH_TRAITS_HPP
