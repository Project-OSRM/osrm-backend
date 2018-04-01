#ifndef MANEUVER_OVERRIDE_RELATION_PARSER_HPP
#define MANEUVER_OVERRIDE_RELATION_PARSER_HPP

#include "maneuver_override.hpp"

#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace osmium
{
class Relation;
}

namespace osrm
{
namespace extractor
{

class ScriptingEnvironment;

/**
 * Parses the relations that represents maneuver overrides.
 * These are structured similarly to turn restrictions, with some slightly
 * different fields.
 *
 * Simple, via-node overrides (the maneuver at the "via" point is overridden)
 * <relation>
 *   <tag k="type" v="maneuver"/>
 *   <member type="way" ref="1234" role="from"/>
 *   <member type="way" ref="5678" role="to"/>
 *   <member type="node" ref="9999" role="via"/>
 *   <tag k="maneuver" v="turn"/>
 *   <tag k="direction" v="slight_right"/>
 * </relation>
 *
 * Via-way descriptions are also supported - this is helpful if
 * you only want to update an instruction if a certain sequence of
 * road transitions are taken.
 *
 * <relation>
 *   <tag k="type" v="maneuver"/>
 *   <member type="way" ref="1234" role="from"/>
 *   <member type="way" ref="5678" role="to"/>
 *   <member type="way" ref="9012" role="via"/> <!-- note via way here -->
 *   <member type="node" ref="9999" role="via"/>
 *   <tag k="maneuver" v="turn"/>
 *   <tag k="direction" v="slight_right"/>
 * </relation>
 *
 * For via-way restrictions, ways must be connected end-to-end, i.e.
 * referenced ways must be split if the turn points are partway
 * along the original way.
 *
 */
class ManeuverOverrideRelationParser
{
  public:
    ManeuverOverrideRelationParser();
    boost::optional<InputManeuverOverride> TryParse(const osmium::Relation &relation) const;
};
}
}

#endif /* RESTRICTION_PARSER_HPP */
