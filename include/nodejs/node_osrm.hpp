#ifndef OSRM_BINDINGS_NODE_HPP
#define OSRM_BINDINGS_NODE_HPP

#include "osrm/osrm_fwd.hpp"

#include <napi.h>

#include <memory>

namespace node_osrm {

class Engine : public Napi::ObjectWrap<Engine> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Engine(const Napi::CallbackInfo& info);

 std::shared_ptr<osrm::OSRM> this_;
private:
   Napi::Value route(const Napi::CallbackInfo& info);
   Napi::Value nearest(const Napi::CallbackInfo& info);
   Napi::Value table(const Napi::CallbackInfo& info);
   Napi::Value tile(const Napi::CallbackInfo& info);
    Napi::Value match(const Napi::CallbackInfo& info);
       Napi::Value trip(const Napi::CallbackInfo& info);
   
//   Napi::Value PlusOne(const Napi::CallbackInfo& info);
//   Napi::Value Multiply(const Napi::CallbackInfo& info);

//   double value_;
};

} // namespace node_osrm

// namespace node_osrm
// {

// struct Engine final : public Nan::ObjectWrap
// {
//     using Base = Nan::ObjectWrap;

//     static NAN_MODULE_INIT(Init);

//     static NAN_METHOD(New);

//     static NAN_METHOD(route);
//     static NAN_METHOD(nearest);
//     static NAN_METHOD(table);
//     static NAN_METHOD(tile);
//     static NAN_METHOD(match);
//     static NAN_METHOD(trip);

//     Engine(osrm::EngineConfig &config);

//     // Thread-safe singleton accessor
//     static Nan::Persistent<v8::Function> &constructor();

//     // Ref-counted OSRM alive even after shutdown until last callback is done
//     std::shared_ptr<osrm::OSRM> this_;
// };

// } // namespace node_osrm

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-parameter"
// NAN_MODULE_WORKER_ENABLED(osrm, node_osrm::Engine::Init)
// #pragma GCC diagnostic pop
 #endif
