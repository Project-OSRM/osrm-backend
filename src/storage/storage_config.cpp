#include "osrm/datasets.hpp"
#include "osrm/exception.hpp"
#include "util/exception_utils.hpp"
#include <boost/algorithm/string/case_conv.hpp>

#include <istream>
#include <string>

namespace osrm::storage
{
std::istream &operator>>(std::istream &in, FeatureDataset &datasets)
{
    std::string token;
    in >> token;
    boost::to_lower(token);

    if (token == "route_steps")
        datasets = FeatureDataset::ROUTE_STEPS;
    else if (token == "route_geometry")
        datasets = FeatureDataset::ROUTE_GEOMETRY;
    else
        throw util::RuntimeError(token, ErrorCode::UnknownFeatureDataset, SOURCE_REF);
    return in;
}

} // namespace osrm::storage
