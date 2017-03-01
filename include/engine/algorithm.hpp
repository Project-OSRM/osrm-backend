#ifndef OSRM_ENGINE_ALGORITHM_HPP
#define OSRM_ENGINE_ALGORITHM_HPP

#include <type_traits>

namespace osrm
{
namespace engine
{
namespace algorithm
{

// Contraction Hiearchy
struct CH final
{
};
// Contraction Hiearchy with core
struct CoreCH final
{
};
// Multi-Level Dijkstra
struct MLD final
{
};

template <typename AlgorithmT> const char *name();
template <> inline const char *name<CH>() { return "CH"; }
template <> inline const char *name<CoreCH>() { return "CoreCH"; }
template <> inline const char *name<MLD>() { return "MLD"; }
}

namespace algorithm_trais
{

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

template <> struct HasAlternativePathSearch<algorithm::CH> final : std::true_type
{
};
template <> struct HasShortestPathSearch<algorithm::CH> final : std::true_type
{
};
template <> struct HasDirectShortestPathSearch<algorithm::CH> final : std::true_type
{
};
template <> struct HasMapMatching<algorithm::CH> final : std::true_type
{
};
template <> struct HasManyToManySearch<algorithm::CH> final : std::true_type
{
};
template <> struct HasGetTileTurns<algorithm::CH> final : std::true_type
{
};

// disbaled because of perfomance reasons
template <> struct HasAlternativePathSearch<algorithm::CoreCH> final : std::false_type
{
};
template <> struct HasManyToManySearch<algorithm::CoreCH> final : std::false_type
{
};
template <> struct HasShortestPathSearch<algorithm::CoreCH> final : std::true_type
{
};
template <> struct HasDirectShortestPathSearch<algorithm::CoreCH> final : std::true_type
{
};
template <> struct HasMapMatching<algorithm::CoreCH> final : std::true_type
{
};
template <> struct HasGetTileTurns<algorithm::CoreCH> final : std::true_type
{
};

// disbaled because of perfomance reasons
template <> struct HasAlternativePathSearch<algorithm::MLD> final : std::false_type
{
};
template <> struct HasManyToManySearch<algorithm::MLD> final : std::false_type
{
};
template <> struct HasShortestPathSearch<algorithm::MLD> final : std::false_type
{
};
template <> struct HasDirectShortestPathSearch<algorithm::MLD> final : std::false_type
{
};
template <> struct HasMapMatching<algorithm::MLD> final : std::false_type
{
};
template <> struct HasGetTileTurns<algorithm::MLD> final : std::false_type
{
};
}
}
}

#endif
