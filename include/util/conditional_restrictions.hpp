#ifndef OSRM_CONDITIONAL_RESTRICTIONS_HPP
#define OSRM_CONDITIONAL_RESTRICTIONS_HPP

#include <string>
#include <vector>

namespace osrm
{
namespace util
{

// Helper functions for OSM conditional restrictions
// http://wiki.openstreetmap.org/wiki/Conditional_restrictions
// Consitional restrictions is a vector of ConditionalRestriction
// with a restriction value and a condition string
struct ConditionalRestriction
{
    std::string value;
    std::string condition;
};

std::vector<ConditionalRestriction> ParseConditionalRestrictions(const std::string &str);

} // util
} // osrm

#endif // OSRM_CONDITIONAL_RESTRICTIONS_HPP
