#ifndef OSRM_ENGINE_ALGORITHM_HPP
#define OSRM_ENGINE_ALGORITHM_HPP

#include <type_traits>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

// Contraction Hiearchy
namespace ch
{
struct Algorithm final
{
};
} // namespace ch
// Multi-Level Dijkstra
namespace mld
{
struct Algorithm final
{
};
} // namespace mld

// Algorithm names
template <typename AlgorithmT> const char *name();
template <> inline const char *name<ch::Algorithm>() { return "CH"; }
template <> inline const char *name<mld::Algorithm>() { return "MLD"; }

// Algorithm identifier
template <typename AlgorithmT> const char *identifier();
template <> inline const char *identifier<ch::Algorithm>() { return "ch"; }
template <> inline const char *identifier<mld::Algorithm>() { return "mld"; }

template <typename AlgorithmT> struct HasAlternativePathSearch final : std::false_type
{
};
template <typename AlgorithmT> struct HasShortestPathSearch final : std::false_type
{
};
template <typename AlgorithmT> struct HasDirectShortestPathSearch final : std::false_type
{
};
template <typename AlgorithmT> struct HasMapMatching final : std::false_type
{
};
template <typename AlgorithmT> struct HasManyToManySearch final : std::false_type
{
};
template <typename AlgorithmT> struct SupportsDistanceAnnotationType final : std::false_type
{
};
template <typename AlgorithmT> struct HasGetTileTurns final : std::false_type
{
};
template <typename AlgorithmT> struct HasExcludeFlags final : std::false_type
{
};

// Algorithms supported by Contraction Hierarchies
template <> struct HasAlternativePathSearch<ch::Algorithm> final : std::true_type
{
};
template <> struct HasShortestPathSearch<ch::Algorithm> final : std::true_type
{
};
template <> struct HasDirectShortestPathSearch<ch::Algorithm> final : std::true_type
{
};
template <> struct HasMapMatching<ch::Algorithm> final : std::true_type
{
};
template <> struct HasManyToManySearch<ch::Algorithm> final : std::true_type
{
};
template <> struct SupportsDistanceAnnotationType<ch::Algorithm> final : std::true_type
{
};
template <> struct HasGetTileTurns<ch::Algorithm> final : std::true_type
{
};
template <> struct HasExcludeFlags<ch::Algorithm> final : std::true_type
{
};

// Algorithms supported by Multi-Level Dijkstra
template <> struct HasAlternativePathSearch<mld::Algorithm> final : std::true_type
{
};
template <> struct HasDirectShortestPathSearch<mld::Algorithm> final : std::true_type
{
};
template <> struct HasShortestPathSearch<mld::Algorithm> final : std::true_type
{
};
template <> struct HasMapMatching<mld::Algorithm> final : std::true_type
{
};
template <> struct HasManyToManySearch<mld::Algorithm> final : std::true_type
{
};
template <> struct SupportsDistanceAnnotationType<mld::Algorithm> final : std::false_type
{
};
template <> struct HasGetTileTurns<mld::Algorithm> final : std::true_type
{
};
template <> struct HasExcludeFlags<mld::Algorithm> final : std::true_type
{
};
} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif
