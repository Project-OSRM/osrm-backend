#ifndef HELLO_WORLD_HPP
#define HELLO_WORLD_HPP

#include "engine/plugins/plugin_base.hpp"

#include "osrm/json_container.hpp"

#include <string>

namespace osrm
{
namespace engine
{
namespace plugins
{

class HelloWorldPlugin final : public BasePlugin
{
  private:
    std::string temp_string;

  public:
    HelloWorldPlugin() : descriptor_string("hello") {}
    virtual ~HelloWorldPlugin() {}
    const std::string GetDescriptor() const override final { return descriptor_string; }

    Status HandleRequest(const RouteParameters &routeParameters,
                         util::json::Object &json_result) override final
    {
        std::string temp_string;
        json_result.values["title"] = "Hello World";

        temp_string = std::to_string(routeParameters.zoom_level);
        json_result.values["zoom_level"] = temp_string;

        temp_string = std::to_string(routeParameters.check_sum);
        json_result.values["check_sum"] = temp_string;
        json_result.values["instructions"] = (routeParameters.print_instructions ? "yes" : "no");
        json_result.values["geometry"] = (routeParameters.geometry ? "yes" : "no");
        json_result.values["compression"] = (routeParameters.compression ? "yes" : "no");
        json_result.values["output_format"] =
            (!routeParameters.output_format.empty() ? "yes" : "no");

        json_result.values["jsonp_parameter"] =
            (!routeParameters.jsonp_parameter.empty() ? "yes" : "no");
        json_result.values["language"] = (!routeParameters.language.empty() ? "yes" : "no");

        temp_string = std::to_string(routeParameters.coordinates.size());
        json_result.values["location_count"] = temp_string;

        util::json::Array json_locations;
        unsigned counter = 0;
        for (const auto coordinate : routeParameters.coordinates)
        {
            util::json::Object json_location;
            util::json::Array json_coordinates;

            json_coordinates.values.push_back(
                static_cast<double>(coordinate.lat / COORDINATE_PRECISION));
            json_coordinates.values.push_back(
                static_cast<double>(coordinate.lon / COORDINATE_PRECISION));
            json_location.values[std::to_string(counter)] = json_coordinates;
            json_locations.values.push_back(json_location);
            ++counter;
        }
        json_result.values["locations"] = json_locations;
        json_result.values["hint_count"] = routeParameters.hints.size();

        util::json::Array json_hints;
        counter = 0;
        for (const std::string &current_hint : routeParameters.hints)
        {
            json_hints.values.push_back(current_hint);
            ++counter;
        }
        json_result.values["hints"] = json_hints;
        return Status::Ok;
    }

  private:
    std::string descriptor_string;
};
}
}
}

#endif // HELLO_WORLD_HPP
