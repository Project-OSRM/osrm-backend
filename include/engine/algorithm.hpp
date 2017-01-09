#ifndef OSRM_ENGINE_ALGORITHM_HPP
#define OSRM_ENGINE_ALGORITHM_HPP

namespace osrm
{
namespace engine
{
namespace algorithm
{

// Contraction Hiearchy
struct CH;
}

namespace algorithm_trais
{
template <typename AlgorithmT> struct HasAlternativeRouting final
{
    template <template <typename A> class FacadeT> bool operator()(const FacadeT<AlgorithmT> &)
    {
        return false;
    }
};

template <> struct HasAlternativeRouting<algorithm::CH> final
{
    template <template <typename A> class FacadeT>
    bool operator()(const FacadeT<algorithm::CH> &facade)
    {
        return facade.GetCoreSize() == 0;
    }
};
}
}
}

#endif
