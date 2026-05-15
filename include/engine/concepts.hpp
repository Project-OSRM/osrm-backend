#ifndef OSRM_ENGINE_CONCEPTS_HPP
#define OSRM_ENGINE_CONCEPTS_HPP

#include "engine/algorithm.hpp"
#include <concepts>

namespace osrm::engine::routing_algorithms
{

/*
 * RoutingAlgorithm concept
 * ------------------------
 * This concept documents and checks the compile-time interface expected
 * from the routing algorithm marker types used throughout the engine
 * (e.g., routing_algorithms::ch::Algorithm and routing_algorithms::mld::Algorithm).
 *
 * Required compile-time/public interface (checked by this concept):
 *  - routing_algorithms::name<Algorithm>() -> const char*
 *  - routing_algorithms::identifier<Algorithm>() -> const char*
 *  - trait templates (instantiable):
 *      HasAlternativePathSearch<Algorithm>
 *      HasShortestPathSearch<Algorithm>
 *      HasDirectShortestPathSearch<Algorithm>
 *      HasMapMatching<Algorithm>
 *      HasManyToManySearch<Algorithm>
 *      SupportsDistanceAnnotationType<Algorithm>
 *      HasGetTileTurns<Algorithm>
 *      HasExcludeFlags<Algorithm>
 *  - the above trait specializations must expose a compile-time ::value convertible to bool
 */
template <typename T>
concept RoutingAlgorithm = requires {
    /* name() and identifier() are callable and return C strings */
    { name<T>() } -> std::convertible_to<const char *>;
    { identifier<T>() } -> std::convertible_to<const char *>;

    /* trait templates are instantiable */
    typename HasAlternativePathSearch<T>;
    typename HasShortestPathSearch<T>;
    typename HasDirectShortestPathSearch<T>;
    typename HasMapMatching<T>;
    typename HasManyToManySearch<T>;
    typename SupportsDistanceAnnotationType<T>;
    typename HasGetTileTurns<T>;
    typename HasExcludeFlags<T>;

    /* trait values are usable as compile-time booleans */
    { HasAlternativePathSearch<T>::value } -> std::convertible_to<bool>;
    { HasShortestPathSearch<T>::value } -> std::convertible_to<bool>;
    { HasDirectShortestPathSearch<T>::value } -> std::convertible_to<bool>;
    { HasMapMatching<T>::value } -> std::convertible_to<bool>;
    { HasManyToManySearch<T>::value } -> std::convertible_to<bool>;
    { SupportsDistanceAnnotationType<T>::value } -> std::convertible_to<bool>;
    { HasGetTileTurns<T>::value } -> std::convertible_to<bool>;
    { HasExcludeFlags<T>::value } -> std::convertible_to<bool>;
} && IsRoutingAlgorithm<T>::value;

} // namespace osrm::engine::routing_algorithms

#endif // OSRM_ENGINE_CONCEPTS_HPP
