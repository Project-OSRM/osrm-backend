#ifndef SCRIPTING_ENVIRONMENT_HPP
#define SCRIPTING_ENVIRONMENT_HPP

#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/restriction.hpp"

#include <osmium/memory/buffer.hpp>

#include <boost/optional/optional.hpp>

#include <tbb/concurrent_vector.h>

#include <string>
#include <vector>

namespace osmium
{
class Node;
class Way;
}

namespace osrm
{

namespace util
{
struct Coordinate;
}

namespace extractor
{

class RestrictionParser;
struct ExtractionNode;
struct ExtractionWay;
struct ExtractionTurn;
struct ExtractionSegment;

/**
 * Abstract class that handles processing osmium ways, nodes and relation objects by applying
 * user supplied profiles.
 */
class ScriptingEnvironment
{
  public:
    ScriptingEnvironment() = default;
    ScriptingEnvironment(const ScriptingEnvironment &) = delete;
    ScriptingEnvironment &operator=(const ScriptingEnvironment &) = delete;
    virtual ~ScriptingEnvironment() = default;

    virtual const ProfileProperties &GetProfileProperties() = 0;

    virtual std::vector<std::string> GetNameSuffixList() = 0;
    virtual std::vector<std::string> GetRestrictions() = 0;
    virtual void SetupSources() = 0;
    virtual void ProcessTurn(ExtractionTurn &turn) = 0;
    virtual void ProcessSegment(ExtractionSegment &segment) = 0;

    virtual void
    ProcessElements(const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
                    const RestrictionParser &restriction_parser,
                    tbb::concurrent_vector<std::pair<std::size_t, ExtractionNode>> &resulting_nodes,
                    tbb::concurrent_vector<std::pair<std::size_t, ExtractionWay>> &resulting_ways,
                    tbb::concurrent_vector<boost::optional<InputRestrictionContainer>>
                        &resulting_restrictions) = 0;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_HPP */
