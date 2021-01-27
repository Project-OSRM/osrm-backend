#ifndef ENGINE_MAP_MATCHING_CONFIDENCE_HPP
#define ENGINE_MAP_MATCHING_CONFIDENCE_HPP

#include "engine/map_matching/bayes_classifier.hpp"

#include <cmath>

namespace osrm
{
namespace engine
{
namespace map_matching
{

struct MatchingConfidence
{
  private:
    using ClassifierT = BayesClassifier<LaplaceDistribution, LaplaceDistribution, double>;
    using TraceClassification = ClassifierT::ClassificationT;

  public:
    MatchingConfidence()
        : // the values were derived from fitting a laplace distribution
          // to the values of manually classified traces
          classifier(map_matching::LaplaceDistribution(0.005986, 0.016646),
                     map_matching::LaplaceDistribution(0.054385, 0.458432),
                     0.696774) // valid apriori probability
    {
    }

    double operator()(const float trace_length, const float matched_length) const
    {
        const double distance_feature = -std::log(trace_length) + std::log(matched_length);

        // matched to the same point
        if (!std::isfinite(distance_feature))
        {
            return 0;
        }

        const auto label_with_confidence = classifier.classify(distance_feature);
        if (label_with_confidence.first == ClassifierT::ClassLabel::POSITIVE)
        {
            return label_with_confidence.second;
        }

        BOOST_ASSERT(label_with_confidence.first == ClassifierT::ClassLabel::NEGATIVE);
        return 1 - label_with_confidence.second;
    }

  private:
    ClassifierT classifier;
};
} // namespace map_matching
} // namespace engine
} // namespace osrm

#endif
