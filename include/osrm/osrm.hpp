/*

Copyright (c) 2016, Project OSRM contributors
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

#ifndef OSRM_HPP
#define OSRM_HPP

#include "osrm/status.hpp"

#include <memory>

namespace osrm
{

// Fwd decls
namespace util
{
namespace json
{
struct Object;
}
}

namespace engine
{
class Engine;
struct EngineConfig;
namespace api
{
struct RouteParameters;
struct TableParameters;
struct NearestParameters;
// struct TripParameters;
// struct MatchParameters;
}
}
// End fwd decls

using engine::EngineConfig;
using engine::api::RouteParameters;
using engine::api::TableParameters;
using engine::api::NearestParameters;
// using engine::api::TripParameters;
// using engine::api::MatchParameters;
namespace json = util::json;

class OSRM
{
  public:
    explicit OSRM(EngineConfig &config);
    ~OSRM();

    OSRM(OSRM &&) noexcept;
    OSRM &operator=(OSRM &&) noexcept;

    Status Route(const RouteParameters &parameters, json::Object &result);
    Status Table(const TableParameters &parameters, json::Object &result);
    Status Nearest(const NearestParameters &parameters, json::Object &result);
    // Status Trip(const TripParameters &parameters, json::Object &result);
    // Status Match(const MatchParameters &parameters, json::Object &result);

  private:
    std::unique_ptr<engine::Engine> engine_;
};
}

#endif // OSRM_HPP
