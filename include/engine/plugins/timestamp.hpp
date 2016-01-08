#ifndef TIMESTAMP_PLUGIN_H
#define TIMESTAMP_PLUGIN_H

#include "engine/plugins/plugin_base.hpp"

#include "osrm/json_container.hpp"

#include <string>

namespace osrm
{
namespace engine
{
namespace plugins
{

template <class DataFacadeT> class TimestampPlugin final : public BasePlugin
{
  public:
    explicit TimestampPlugin(const DataFacadeT *facade)
        : facade(facade), descriptor_string("timestamp")
    {
    }
    const std::string GetDescriptor() const override final { return descriptor_string; }
    Status HandleRequest(const RouteParameters &route_parameters,
                         util::json::Object &json_result) override final
    {
        (void)route_parameters; // unused

        const std::string timestamp = facade->GetTimestamp();
        json_result.values["timestamp"] = timestamp;
        return Status::Ok;
    }

  private:
    const DataFacadeT *facade;
    std::string descriptor_string;
};
}
}
}

#endif /* TIMESTAMP_PLUGIN_H */
