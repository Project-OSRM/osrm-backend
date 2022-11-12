#ifndef OSRM_BINDINGS_NODE_HPP
#define OSRM_BINDINGS_NODE_HPP

#include "osrm/osrm_fwd.hpp"

#include <napi.h>

#include <memory>

namespace node_osrm
{

class Engine final : public Napi::ObjectWrap<Engine>
{
  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    Engine(const Napi::CallbackInfo &info);

    std::shared_ptr<osrm::OSRM> this_;

  private:
    Napi::Value route(const Napi::CallbackInfo &info);
    Napi::Value nearest(const Napi::CallbackInfo &info);
    Napi::Value table(const Napi::CallbackInfo &info);
    Napi::Value tile(const Napi::CallbackInfo &info);
    Napi::Value match(const Napi::CallbackInfo &info);
    Napi::Value trip(const Napi::CallbackInfo &info);
};

} // namespace node_osrm

#endif
