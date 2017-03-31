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
}
// Contraction Hiearchy with core
namespace corech
{
struct Algorithm final
{
};
}
// Multi-Level Dijkstra
namespace mld
{
struct Algorithm final
{
};
}

// Algorithm names
template <typename AlgorithmT> const char *name();
template <> inline const char *name<ch::Algorithm>() { return "CH"; }
template <> inline const char *name<corech::Algorithm>() { return "CoreCH"; }
template <> inline const char *name<mld::Algorithm>() { return "MLD"; }

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
template <typename AlgorithmT> struct HasGetTileTurns final : std::false_type
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
template <> struct HasGetTileTurns<ch::Algorithm> final : std::true_type
{
};

// Algorithms supported by Contraction Hierarchies with core
// the rest is disabled because of performance reasons
template <> struct HasShortestPathSearch<corech::Algorithm> final : std::true_type
{
};
template <> struct HasDirectShortestPathSearch<corech::Algorithm> final : std::true_type
{
};
template <> struct HasMapMatching<corech::Algorithm> final : std::true_type
{
};
template <> struct HasGetTileTurns<corech::Algorithm> final : std::true_type
{
};

// Algorithms supported by Multi-Level Dijkstra
template <> struct HasDirectShortestPathSearch<mld::Algorithm> final : std::true_type
{
};
template <> struct HasShortestPathSearch<mld::Algorithm> final : std::true_type
{
};
template <> struct HasMapMatching<mld::Algorithm> final : std::true_type
{
};
}
}
}

#endif
