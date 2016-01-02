/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef BASE_PLUGIN_HPP
#define BASE_PLUGIN_HPP

#include "../data_structures/phantom_node.hpp"

#include <osrm/coordinate.hpp>
#include <osrm/json_container.hpp>
#include <osrm/route_parameters.hpp>

#include <algorithm>
#include <string>
#include <vector>

class BasePlugin
{
  public:
    enum class Status : int
    {
      Ok = 200,
      EmptyResult = 207,
      NoSegment = 208,
      Error = 400
    };

    BasePlugin() {}
    // Maybe someone can explain the pure virtual destructor thing to me (dennis)
    virtual ~BasePlugin() {}
    virtual const std::string GetDescriptor() const = 0;
    virtual Status HandleRequest(const RouteParameters &, osrm::json::Object &) = 0;
    virtual bool check_all_coordinates(const std::vector<FixedPointCoordinate> &coordinates,
                                       const unsigned min = 2) const final
    {
        if (min > coordinates.size() || std::any_of(std::begin(coordinates), std::end(coordinates),
                                                    [](const FixedPointCoordinate &coordinate)
                                                    {
                                                        return !coordinate.is_valid();
                                                    }))
        {
            return false;
        }
        return true;
    }

    // Decides whether to use the phantom node from a big or small component if both are found.
    // Returns true if all phantom nodes are in the same component after snapping.
    std::vector<PhantomNode> snapPhantomNodes(
        const std::vector<std::pair<PhantomNode, PhantomNode>> &phantom_node_pair_list) const
    {
        const auto check_component_id_is_tiny =
            [](const std::pair<PhantomNode, PhantomNode> &phantom_pair)
        {
            return phantom_pair.first.component.is_tiny;
        };

        // are all phantoms from a tiny cc?
        const auto check_all_in_same_component =
            [](const std::vector<std::pair<PhantomNode, PhantomNode>> &nodes)
        {
            const auto component_id = nodes.front().first.component.id;

            return std::all_of(std::begin(nodes), std::end(nodes),
                               [component_id](const PhantomNodePair &phantom_pair)
                               {
                                   return component_id == phantom_pair.first.component.id;
                               });
        };

        const auto fallback_to_big_component =
            [](const std::pair<PhantomNode, PhantomNode> &phantom_pair)
        {
            if (phantom_pair.first.component.is_tiny && phantom_pair.second.is_valid() &&
                !phantom_pair.second.component.is_tiny)
            {
                return phantom_pair.second;
            }
            return phantom_pair.first;
        };

        const auto use_closed_phantom = [](const std::pair<PhantomNode, PhantomNode> &phantom_pair)
        {
            return phantom_pair.first;
        };

        const bool every_phantom_is_in_tiny_cc =
            std::all_of(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                        check_component_id_is_tiny);
        auto all_in_same_component = check_all_in_same_component(phantom_node_pair_list);

        std::vector<PhantomNode> snapped_phantoms;
        snapped_phantoms.reserve(phantom_node_pair_list.size());

        // The only case we don't snap to the big component if all phantoms are in the same small
        // component
        if (every_phantom_is_in_tiny_cc && all_in_same_component)
        {
            std::transform(phantom_node_pair_list.begin(), phantom_node_pair_list.end(),
                           std::back_inserter(snapped_phantoms), use_closed_phantom);
        }
        else
        {
            std::transform(phantom_node_pair_list.begin(), phantom_node_pair_list.end(),
                           std::back_inserter(snapped_phantoms), fallback_to_big_component);
        }

        return snapped_phantoms;
    }
};

#endif /* BASE_PLUGIN_HPP */
