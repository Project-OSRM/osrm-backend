#ifndef MOCK_SCRIPTING_ENVIRONMENT_HPP_
#define MOCK_SCRIPTING_ENVIRONMENT_HPP_

#include "extractor/extraction_segment.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/scripting_environment.hpp"

#include <string>
#include <utility>
#include <vector>

namespace osrm
{

namespace test
{

// a mock implementation of the scripting environment doing exactly nothing
class MockScriptingEnvironment : public extractor::ScriptingEnvironment
{

    const extractor::ProfileProperties &GetProfileProperties() override final
    {
        static extractor::ProfileProperties properties;
        return properties;
    }

    std::vector<std::string> GetNameSuffixList() override final { return {}; }
    std::vector<std::vector<std::string>> GetExcludableClasses() override final { return {}; };
    std::vector<std::string> GetClassNames() override { return {}; };

    std::vector<std::string> GetRestrictions() override final { return {}; }
    void ProcessTurn(extractor::ExtractionTurn &) override final {}
    void ProcessSegment(extractor::ExtractionSegment &) override final {}

    void ProcessElements(
        const osmium::memory::Buffer &,
        const extractor::RestrictionParser &,
        const extractor::ExtractionRelationContainer &,
        std::vector<std::pair<const osmium::Node &, extractor::ExtractionNode>> &,
        std::vector<std::pair<const osmium::Way &, extractor::ExtractionWay>> &,
        std::vector<std::pair<const osmium::Relation &, extractor::ExtractionRelation>> &,
        std::vector<extractor::InputConditionalTurnRestriction> &) override final
    {
    }

    bool HasLocationDependentData() const { return false; };
};

} // namespace test
} // namespace osrm

#endif // MOCK_SCRIPTING_ENVIRONMENT_HPP_
