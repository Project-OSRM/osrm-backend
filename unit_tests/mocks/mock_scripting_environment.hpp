#ifndef MOCK_SCRIPTING_ENVIRONMENT_HPP_
#define MOCK_SCRIPTING_ENVIRONMENT_HPP_

#include "extractor/extraction_segment.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/scripting_environment.hpp"

#include <string>
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

    std::vector<std::string> GetNameSuffixList() override final { return {}; }
    std::vector<std::vector<std::string>> GetExcludableClasses() override final { return {}; };
    std::vector<std::string> GetClassNames() override { return {}; };
    std::vector<std::string> GetRelations() override { return {}; };

    std::vector<std::string> GetRestrictions() override final { return {}; }
    void ProcessTurn(extractor::ExtractionTurn &) override final {}
    void ProcessSegment(extractor::ExtractionSegment &) override final {}

    void ProcessRelation(extractor::ScriptingResults &) override final {};
    void ProcessElements(extractor::ScriptingResults &,
                         const extractor::RestrictionParser &,
                         const extractor::ManeuverOverrideRelationParser &) override final
    {
    }

    bool HasLocationDependentData() const override { return false; };
};

} // namespace osrm::test

#endif // MOCK_SCRIPTING_ENVIRONMENT_HPP_
