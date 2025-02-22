#ifndef BASE_PLUGIN_HPP
#define BASE_PLUGIN_HPP

#include "engine/api/base_parameters.hpp"
#include "engine/api/base_result.hpp"
#include "engine/api/flatbuffers/fbresult_generated.h"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/phantom_node.hpp"
#include "engine/routing_algorithms.hpp"
#include "engine/status.hpp"

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/integer_range.hpp"
#include "util/json_container.hpp"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include <util/log.hpp>

namespace osrm::engine::plugins
{

class BasePlugin
{
  protected:
    BasePlugin() = default;

    BasePlugin(const std::optional<double> default_radius_) : default_radius(default_radius_) {}

    bool CheckAllCoordinates(const std::vector<util::Coordinate> &coordinates) const
    {
        return !std::any_of(std::begin(coordinates),
                            std::end(coordinates),
                            [](const util::Coordinate coordinate)
                            { return !coordinate.IsValid(); });
    }

    bool CheckAlgorithms(const api::BaseParameters &params,
                         const RoutingAlgorithmsInterface &algorithms,
                         osrm::engine::api::ResultT &result) const
    {
        if (algorithms.IsValid())
        {
            return true;
        }

        if (!algorithms.HasExcludeFlags() && !params.exclude.empty())
        {
            Error("NotImplemented", "This algorithm does not support exclude flags.", result);
            return false;
        }
        if (algorithms.HasExcludeFlags() && !params.exclude.empty())
        {
            Error("InvalidValue", "Exclude flag combination is not supported.", result);
            return false;
        }

        BOOST_ASSERT_MSG(false,
                         "There are only two reasons why the algorithm interface can be invalid.");
        return false;
    }

    struct ErrorRenderer
    {
        std::string code;
        std::string message;

        ErrorRenderer(std::string code, std::string message)
            : code(std::move(code)), message(std::move(message)){};

        void operator()(util::json::Object &json_result)
        {
            json_result.values["code"] = code;
            json_result.values["message"] = message;
        };
        void operator()(flatbuffers::FlatBufferBuilder &fb_result)
        {
            auto error = api::fbresult::CreateErrorDirect(fb_result, code.c_str(), message.c_str());
            api::fbresult::FBResultBuilder response(fb_result);
            response.add_error(true);
            response.add_code(error);
            fb_result.Finish(response.Finish());
        };
        void operator()(std::string &str_result)
        {
            str_result = str(boost::format("code=%1% message=%2%") % code % message);
        };
    };

    Status Error(const std::string &code,
                 const std::string &message,
                 osrm::engine::api::ResultT &result) const
    {
        std::visit(ErrorRenderer(code, message), result);
        return Status::Error;
    }

    // Decides whether to use the phantom candidates from big or small components if both are found.
    std::vector<PhantomNodeCandidates>
    SnapPhantomNodes(std::vector<PhantomCandidateAlternatives> alternatives_list) const
    {
        // are all phantoms from a tiny cc?
        const auto all_in_same_tiny_component =
            [](const std::vector<PhantomCandidateAlternatives> &alts_list)
        {
            return std::any_of(
                alts_list.front().first.begin(),
                alts_list.front().first.end(),
                // For each of the first possible phantoms, check if all other
                // positions in the list have a phantom from the same small component.
                [&](const PhantomNode &phantom)
                {
                    if (!phantom.component.is_tiny)
                    {
                        return false;
                    }
                    const auto component_id = phantom.component.id;
                    return std::all_of(
                        std::next(alts_list.begin()),
                        std::end(alts_list),
                        [component_id](const PhantomCandidateAlternatives &alternatives)
                        { return candidatesHaveComponent(alternatives.first, component_id); });
                });
        };

        // Move the alternative into the final list
        const auto fallback_to_big_component = [](PhantomCandidateAlternatives &alternatives)
        {
            auto no_big_alternative = alternatives.second.empty();
            return no_big_alternative ? std::move(alternatives.first)
                                      : std::move(alternatives.second);
        };

        // Move the alternative into the final list
        const auto use_closed_phantom = [](PhantomCandidateAlternatives &alternatives)
        { return std::move(alternatives.first); };

        const auto no_alternatives =
            std::all_of(alternatives_list.begin(),
                        alternatives_list.end(),
                        [](const PhantomCandidateAlternatives &alternatives)
                        { return alternatives.second.empty(); });

        std::vector<PhantomNodeCandidates> snapped_phantoms;
        snapped_phantoms.reserve(alternatives_list.size());
        // The only case we don't snap to the big component if all phantoms are in the same small
        // component
        if (no_alternatives || all_in_same_tiny_component(alternatives_list))
        {
            std::transform(alternatives_list.begin(),
                           alternatives_list.end(),
                           std::back_inserter(snapped_phantoms),
                           use_closed_phantom);
        }
        else
        {
            std::transform(alternatives_list.begin(),
                           alternatives_list.end(),
                           std::back_inserter(snapped_phantoms),
                           fallback_to_big_component);
        }

        return snapped_phantoms;
    }

    // Falls back to default_radius for non-set radii
    std::vector<std::vector<PhantomNodeWithDistance>>
    GetPhantomNodesInRange(const datafacade::BaseDataFacade &facade,
                           const api::BaseParameters &parameters,
                           const std::vector<double> &radiuses,
                           bool use_all_edges = false) const
    {
        std::vector<std::vector<PhantomNodeWithDistance>> phantom_nodes(
            parameters.coordinates.size());
        BOOST_ASSERT(radiuses.size() == parameters.coordinates.size());

        const bool use_hints = !parameters.hints.empty();
        const bool use_bearings = !parameters.bearings.empty();
        const bool use_approaches = !parameters.approaches.empty();

        for (const auto i : util::irange<std::size_t>(0UL, parameters.coordinates.size()))
        {
            if (use_hints && parameters.hints[i] && !parameters.hints[i]->segment_hints.empty() &&
                parameters.hints[i]->IsValid(parameters.coordinates[i], facade))
            {
                for (const auto &seg_hint : parameters.hints[i]->segment_hints)
                {
                    phantom_nodes[i].push_back(PhantomNodeWithDistance{
                        seg_hint.phantom,
                        util::coordinate_calculation::greatCircleDistance(
                            parameters.coordinates[i], seg_hint.phantom.location)});
                }
                continue;
            }

            phantom_nodes[i] = facade.NearestPhantomNodesInRange(
                parameters.coordinates[i],
                radiuses[i],
                use_bearings ? parameters.bearings[i] : std::nullopt,
                use_approaches && parameters.approaches[i] ? parameters.approaches[i].value()
                                                           : engine::Approach::UNRESTRICTED,
                use_all_edges);
        }

        return phantom_nodes;
    }

    std::vector<std::vector<PhantomNodeWithDistance>>
    GetPhantomNodes(const datafacade::BaseDataFacade &facade,
                    const api::BaseParameters &parameters,
                    size_t number_of_results) const
    {
        std::vector<std::vector<PhantomNodeWithDistance>> phantom_nodes(
            parameters.coordinates.size());

        const bool use_hints = !parameters.hints.empty();
        const bool use_bearings = !parameters.bearings.empty();
        const bool use_radiuses = !parameters.radiuses.empty();
        const bool use_approaches = !parameters.approaches.empty();

        BOOST_ASSERT(parameters.IsValid());
        for (const auto i : util::irange<std::size_t>(0UL, parameters.coordinates.size()))
        {
            if (use_hints && parameters.hints[i] && !parameters.hints[i]->segment_hints.empty() &&
                parameters.hints[i]->IsValid(parameters.coordinates[i], facade))
            {
                for (const auto &seg_hint : parameters.hints[i]->segment_hints)
                {
                    phantom_nodes[i].push_back(PhantomNodeWithDistance{
                        seg_hint.phantom,
                        util::coordinate_calculation::greatCircleDistance(
                            parameters.coordinates[i], seg_hint.phantom.location)});
                }
                continue;
            }

            phantom_nodes[i] = facade.NearestPhantomNodes(
                parameters.coordinates[i],
                number_of_results,
                use_radiuses ? parameters.radiuses[i] : default_radius,
                use_bearings ? parameters.bearings[i] : std::nullopt,
                use_approaches && parameters.approaches[i] ? parameters.approaches[i].value()
                                                           : engine::Approach::UNRESTRICTED);

            // we didn't find a fitting node, return error
            if (phantom_nodes[i].empty())
            {
                break;
            }
        }
        return phantom_nodes;
    }

    std::vector<PhantomCandidateAlternatives>
    GetPhantomNodes(const datafacade::BaseDataFacade &facade,
                    const api::BaseParameters &parameters) const
    {
        std::vector<PhantomCandidateAlternatives> alternatives(parameters.coordinates.size());

        const bool use_hints = !parameters.hints.empty();
        const bool use_bearings = !parameters.bearings.empty();
        const bool use_radiuses = !parameters.radiuses.empty();
        const bool use_approaches = !parameters.approaches.empty();
        const bool use_all_edges = parameters.snapping == api::BaseParameters::SnappingType::Any;

        BOOST_ASSERT(parameters.IsValid());
        for (const auto i : util::irange<std::size_t>(0UL, parameters.coordinates.size()))
        {
            if (use_hints && parameters.hints[i] && !parameters.hints[i]->segment_hints.empty() &&
                parameters.hints[i]->IsValid(parameters.coordinates[i], facade))
            {
                std::transform(parameters.hints[i]->segment_hints.begin(),
                               parameters.hints[i]->segment_hints.end(),
                               std::back_inserter(alternatives[i].first),
                               [](const auto &seg_hint) { return seg_hint.phantom; });
                // we don't set the second one - it will be marked as invalid
                continue;
            }

            alternatives[i] = facade.NearestCandidatesWithAlternativeFromBigComponent(
                parameters.coordinates[i],
                use_radiuses ? parameters.radiuses[i] : default_radius,
                use_bearings ? parameters.bearings[i] : std::nullopt,
                use_approaches && parameters.approaches[i] ? parameters.approaches[i].value()
                                                           : engine::Approach::UNRESTRICTED,
                use_all_edges);

            // we didn't find a fitting node, return error
            if (alternatives[i].first.empty())
            {
                // This ensures the list of phantom nodes only consists of valid nodes.
                // We can use this on the call-site to detect an error.
                alternatives.pop_back();
                break;
            }

            BOOST_ASSERT(!alternatives[i].first.empty());
        }
        return alternatives;
    }

    std::string
    MissingPhantomErrorMessage(const std::vector<PhantomCandidateAlternatives> &alternatives,
                               const std::vector<util::Coordinate> &coordinates) const
    {
        BOOST_ASSERT(alternatives.size() < coordinates.size());
        auto mismatch =
            std::mismatch(alternatives.begin(),
                          alternatives.end(),
                          coordinates.begin(),
                          coordinates.end(),
                          [](const auto &candidates_pair, const auto &coordinate)
                          {
                              return std::any_of(candidates_pair.first.begin(),
                                                 candidates_pair.first.end(),
                                                 [&](const auto &phantom)
                                                 { return phantom.input_location == coordinate; });
                          });
        std::size_t missing_index = std::distance(alternatives.begin(), mismatch.first);
        return std::string("Could not find a matching segment for coordinate ") +
               std::to_string(missing_index);
    }

    const std::optional<double> default_radius;
};
} // namespace osrm::engine::plugins

#endif /* BASE_PLUGIN_HPP */
