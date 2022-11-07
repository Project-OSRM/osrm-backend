

#include "osrm/engine_config.hpp"
#include "osrm/osrm.hpp"

#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/tile_parameters.hpp"
#include "osrm/trip_parameters.hpp"

#include <exception>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "nodejs/node_osrm.hpp"
#include "nodejs/node_osrm_support.hpp"

#include "util/json_renderer.hpp"

namespace node_osrm {
Napi::Object Engine::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func =
      DefineClass(env,
                  "OSRM",
                  {InstanceMethod("route", &Engine::route)});

  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("OSRM", func);
  return exports;
}

Engine::Engine(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<Engine>(info) {

        try
        {
            auto config = argumentsToEngineConfig(info);
            if (!config)
                return;

           this_ = std::make_shared<osrm::OSRM>(std::move(*config));
        }
        catch (const std::exception &ex)
        {
            return ThrowTypeError(info.Env(), ex.what());
        }
}

template <typename ParameterParser, typename ServiceMemFn>
inline void async(const Napi::CallbackInfo& info,
                  ParameterParser argsToParams,
                  ServiceMemFn service,
                  bool requires_multiple_coordinates)
{
    auto params = argsToParams(info, requires_multiple_coordinates);
    if (!params)
        return;
    auto pluginParams = argumentsToPluginParameters(info, params->format);

    BOOST_ASSERT(params->IsValid());

    if (!info[info.Length() - 1].IsFunction())
        return ThrowTypeError(info.Env(), "last argument must be a callback function");

    auto *const self = Napi::ObjectWrap::Unwrap<Engine>(info.Holder());
    using ParamPtr = decltype(params);

    struct Worker final : Napi::AsyncWorker
    {
        Worker(std::shared_ptr<osrm::OSRM> osrm_,
               ParamPtr params_,
               ServiceMemFn service,
               Napi::Function callback,
               PluginParameters pluginParams_)
            : Napi::AsyncWorker(callback), osrm{std::move(osrm_)},
              service{std::move(service)}, params{std::move(params_)}, pluginParams{
                                                                           std::move(pluginParams_)}
        {
        }

        void Execute() override
        try
        {
            switch (
                params->format.value_or(osrm::engine::api::BaseParameters::OutputFormatType::JSON))
            {
            case osrm::engine::api::BaseParameters::OutputFormatType::JSON:
            {
                osrm::engine::api::ResultT r;
                r = osrm::util::json::Object();
                const auto status = ((*osrm).*(service))(*params, r);
                auto &json_result = r.get<osrm::json::Object>();
                ParseResult(status, json_result);
                if (pluginParams.renderToBuffer)
                {
                    std::string json_string;
                    osrm::util::json::render(json_string, json_result);
                    result = std::move(json_string);
                }
                else
                {
                    result = std::move(json_result);
                }
            }
            break;
            case osrm::engine::api::BaseParameters::OutputFormatType::FLATBUFFERS:
            {
                osrm::engine::api::ResultT r = flatbuffers::FlatBufferBuilder();
                const auto status = ((*osrm).*(service))(*params, r);
                const auto &fbs_result = r.get<flatbuffers::FlatBufferBuilder>();
                ParseResult(status, fbs_result);
                BOOST_ASSERT(pluginParams.renderToBuffer);
                std::string result_str(
                    reinterpret_cast<const char *>(fbs_result.GetBufferPointer()),
                    fbs_result.GetSize());
                result = std::move(result_str);
            }
            break;
            }
        }
        catch (const std::exception &e)
        {
            SetError(e.what());
        }

        void OnOK() override
        {
            Napi::HandleScope scope(Napi::Env());

            Callback().Call({Env().Null(), render(result)});
        }

        // Keeps the OSRM object alive even after shutdown until we're done with callback
        std::shared_ptr<osrm::OSRM> osrm;
        ServiceMemFn service;
        const ParamPtr params;
        const PluginParameters pluginParams;

        ObjectOrString result;
    };

    Napi::Function callback = info[info.Length() - 1].As<Napi::Function>();
    auto worker = new Worker(self->this_, std::move(params), service, callback, std::move(pluginParams));
    worker->Queue();
}

Napi::Value Engine::route(const Napi::CallbackInfo& info) {
    osrm::Status (osrm::OSRM::*route_fn)(const osrm::RouteParameters &params,
                                         osrm::engine::api::ResultT &result) const =
        &osrm::OSRM::Route;
    async(info, &argumentsToRouteParameter, route_fn, true);
    return info.Env().Undefined();
}

} // namespace node_osrm

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return node_osrm::Engine::Init(env, exports);
}

NODE_API_MODULE(addon, InitAll);

