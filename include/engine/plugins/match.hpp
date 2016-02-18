#ifndef MATCH_HPP
#define MATCH_HPP

#include "engine/plugins/plugin_base.hpp"
#include "engine/api/match_parameters.hpp"

#include "engine/map_matching/bayes_classifier.hpp"
#include "engine/routing_algorithms/map_matching.hpp"
#include "util/json_util.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace plugins
{

class MatchingPlugin : public BasePlugin
{
    using SubMatching = routing_algorithms::SubMatching;
    using SubMatchingList = routing_algorithms::SubMatchingList;
    using CandidateLists = routing_algorithms::CandidateLists;
    using ClassifierT = map_matching::BayesClassifier<map_matching::LaplaceDistribution,
                                                      map_matching::LaplaceDistribution,
                                                      double>;
    using TraceClassification = ClassifierT::ClassificationT;

  public:
    MatchingPlugin(datafacade::BaseDataFacade &facade_, const int max_locations_map_matching)
        : BasePlugin(facade_),
          max_locations_map_matching(max_locations_map_matching),
          // the values were derived from fitting a laplace distribution
          // to the values of manually classified traces
          classifier(map_matching::LaplaceDistribution(0.005986, 0.016646),
                     map_matching::LaplaceDistribution(0.054385, 0.458432),
                     0.696774) // valid apriori probability
    {
    }

    Status HandleRequest(const MatchParameters &parameters, util::json::Object &json_result);

  private:
    int max_locations_map_matching;
    ClassifierT classifier;
};
}
}
}

#endif // MATCH_HPP
