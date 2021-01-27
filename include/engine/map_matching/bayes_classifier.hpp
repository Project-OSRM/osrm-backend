#ifndef BAYES_CLASSIFIER_HPP
#define BAYES_CLASSIFIER_HPP

#include <cmath>

#include <utility>
#include <vector>

#include <boost/math/constants/constants.hpp>

namespace osrm
{
namespace engine
{
namespace map_matching
{

struct NormalDistribution
{
    NormalDistribution(const double mean, const double standard_deviation)
        : mean(mean), standard_deviation(standard_deviation)
    {
    }

    // FIXME implement log-probability version since it's faster
    double Density(const double val) const
    {
        using namespace boost::math::constants;

        const double x = val - mean;
        return 1.0 / (std::sqrt(two_pi<double>()) * standard_deviation) *
               std::exp(-x * x / (standard_deviation * standard_deviation));
    }

    double mean;
    double standard_deviation;
};

struct LaplaceDistribution
{
    LaplaceDistribution(const double location, const double scale)
        : location(location), scale(scale)
    {
    }

    // FIXME implement log-probability version since it's faster
    double Density(const double val) const
    {
        const double x = std::abs(val - location);
        return 1.0 / (2. * scale) * std::exp(-x / scale);
    }

    double location;
    double scale;
};

template <typename PositiveDistributionT, typename NegativeDistributionT, typename ValueT>
class BayesClassifier
{
  public:
    enum class ClassLabel : unsigned
    {
        NEGATIVE,
        POSITIVE
    };
    using ClassificationT = std::pair<ClassLabel, double>;

    BayesClassifier(PositiveDistributionT positive_distribution,
                    NegativeDistributionT negative_distribution,
                    const double positive_apriori_probability)
        : positive_distribution(std::move(positive_distribution)),
          negative_distribution(std::move(negative_distribution)),
          positive_apriori_probability(positive_apriori_probability),
          negative_apriori_probability(1. - positive_apriori_probability)
    {
    }

    // Returns label and the probability of the label.
    ClassificationT classify(const ValueT &v) const
    {
        const double positive_postpriori =
            positive_apriori_probability * positive_distribution.Density(v);
        const double negative_postpriori =
            negative_apriori_probability * negative_distribution.Density(v);
        const double norm = positive_postpriori + negative_postpriori;

        if (positive_postpriori > negative_postpriori)
        {
            return std::make_pair(ClassLabel::POSITIVE, positive_postpriori / norm);
        }

        return std::make_pair(ClassLabel::NEGATIVE, negative_postpriori / norm);
    }

  private:
    PositiveDistributionT positive_distribution;
    NegativeDistributionT negative_distribution;
    double positive_apriori_probability;
    double negative_apriori_probability;
};
} // namespace map_matching
} // namespace engine
} // namespace osrm

#endif // BAYES_CLASSIFIER_HPP
