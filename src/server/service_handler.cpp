#include "server/service_handler.hpp"

#include "server/service/match_service.hpp"
#include "server/service/nearest_service.hpp"
#include "server/service/route_service.hpp"
#include "server/service/table_service.hpp"
#include "server/service/tile_service.hpp"
#include "server/service/trip_service.hpp"

#include "server/api/parsed_url.hpp"
#include "util/json_util.hpp"

namespace osrm::server
{
ServiceHandler::ServiceHandler(osrm::EngineConfig &config) : routing_machine(config)
{
    service_map["route"] = std::make_unique<service::RouteService>(routing_machine);
    service_map["table"] = std::make_unique<service::TableService>(routing_machine);
    service_map["nearest"] = std::make_unique<service::NearestService>(routing_machine);
    service_map["trip"] = std::make_unique<service::TripService>(routing_machine);
    service_map["match"] = std::make_unique<service::MatchService>(routing_machine);
    service_map["tile"] = std::make_unique<service::TileService>(routing_machine);
}

namespace
{
// Resolves the service for a parsed URL. On failure writes a JSON error into `result` and
// returns nullptr.
service::BaseService *
resolveService(std::unordered_map<std::string, std::unique_ptr<service::BaseService>> &service_map,
               const api::ParsedURL &parsed_url,
               osrm::engine::api::ResultT &result)
{
    const auto service_iter = service_map.find(parsed_url.service);
    if (service_iter == service_map.end())
    {
        result = util::json::Object();
        auto &json_result = std::get<util::json::Object>(result);
        json_result.values["code"] = "InvalidService";
        json_result.values["message"] = "Service " + parsed_url.service + " not found!";
        return nullptr;
    }
    auto &service = service_iter->second;

    if (service->GetVersion() != parsed_url.version)
    {
        result = util::json::Object();
        auto &json_result = std::get<util::json::Object>(result);
        json_result.values["code"] = "InvalidVersion";
        json_result.values["message"] = "Service " + parsed_url.service + " not found!";
        return nullptr;
    }

    return service.get();
}
} // namespace

engine::Status ServiceHandler::RunQuery(api::ParsedURL parsed_url,
                                        osrm::engine::api::ResultT &result)
{
    auto *service = resolveService(service_map, parsed_url, result);
    if (!service)
        return engine::Status::Error;

    return service->RunQuery(parsed_url.prefix_length, parsed_url.query, result);
}

engine::Status ServiceHandler::RunQuery(api::ParsedURL parsed_url,
                                        const std::string &json_body,
                                        osrm::engine::api::ResultT &result)
{
    auto *service = resolveService(service_map, parsed_url, result);
    if (!service)
        return engine::Status::Error;

    return service->RunJSONQuery(json_body, result);
}
} // namespace osrm::server
