#ifndef MOCK_SCRIPTING_ENVIRONMENT_HPP_
#define MOCK_SCRIPTING_ENVIRONMENT_HPP_

#include "extractor/extraction_segment.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/scripting_environment.hpp"

#include <string>
#include <utility>
#include <vector>

namespace osrm::test
{

// a mock implementation of the scripting environment doing exactly nothing
class MockScriptingEnvironment : public extractor::ScriptingEnvironment
{

    const extractor::ProfileProperties &GetProfileProperties() override final
    {
        static extractor::ProfileProperties properties;
        return properties;
    }
    bool HasRelationFunction() override final { return false; };

    std::vector<std::string> GetNameSuffixList() override final { return {}; }
    std::vector<std::vector<std::string>> GetExcludableClasses() override final { return {}; };
    std::vector<std::string> GetClassNames() override { return {}; };
    std::vector<std::string> GetRelations() override { return {}; };

    std::vector<std::string> GetRestrictions() override final { return {}; }
    void ProcessTurn(extractor::ExtractionTurn &) override final {}
    void ProcessSegment(extractor::ExtractionSegment &) override final {}

    void ProcessRelation(const osmium::memory::Buffer &,
                         const extractor::ExtractionRelationContainer &,
                         extractor::ScriptingResults &) override final{};
    void ProcessElements(const osmium::memory::Buffer &,
                         const extractor::RestrictionParser &,
                         const extractor::ManeuverOverrideRelationParser &,
                         const extractor::ExtractionRelationContainer &,
                         extractor::ScriptingResults &) override final
    {
    }

    bool HasLocationDependentData() const override { return false; };
};

} // namespace osrm::test

#endif // MOCK_SCRIPTING_ENVIRONMENT_HPP_
