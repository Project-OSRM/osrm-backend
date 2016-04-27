#ifndef SEARCH_ENGINE_HPP
#define SEARCH_ENGINE_HPP

#include "engine/search_engine_data.hpp"
#include "engine/routing_algorithms/alternative_path.hpp"
#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/routing_algorithms/map_matching.hpp"
#include "engine/routing_algorithms/shortest_path.hpp"
#include "engine/routing_algorithms/direct_shortest_path.hpp"
#include "engine/routing_algorithms/one_to_many.hpp"

#include <type_traits>

namespace osrm
{
namespace engine
{

template <class DataFacadeT> class SearchEngine
{
  private:
    DataFacadeT *facade;
    SearchEngineData engine_working_data;

  public:
    routing_algorithms::ShortestPathRouting<DataFacadeT> shortest_path;
    routing_algorithms::DirectShortestPathRouting<DataFacadeT> direct_shortest_path;
    routing_algorithms::AlternativeRouting<DataFacadeT> alternative_path;
    routing_algorithms::ManyToManyRouting<DataFacadeT> distance_table;
    routing_algorithms::MapMatching<DataFacadeT> map_matching;
    routing_algorithms::OneToManyRouting<DataFacadeT> oneToMany;

    explicit SearchEngine(DataFacadeT *facade)
        : facade(facade), shortest_path(facade, engine_working_data),
          direct_shortest_path(facade, engine_working_data),
          alternative_path(facade, engine_working_data),
          distance_table(facade, engine_working_data), map_matching(facade, engine_working_data),
          oneToMany(facade, engine_working_data)
    {
        static_assert(!std::is_pointer<DataFacadeT>::value, "don't instantiate with ptr type");
        static_assert(std::is_object<DataFacadeT>::value,
                      "don't instantiate with void, function, or reference");
    }

    ~SearchEngine() {}
};
}
}

#endif // SEARCH_ENGINE_HPP
