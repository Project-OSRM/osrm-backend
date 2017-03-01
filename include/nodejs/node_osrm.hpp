#ifndef OSRM_BINDINGS_NODE_HPP
#define OSRM_BINDINGS_NODE_HPP

#include "osrm/osrm_fwd.hpp"

#include <nan.h>

#include <memory>

namespace node_osrm
{

struct Engine final : public Nan::ObjectWrap
{
    using Base = Nan::ObjectWrap;

    static NAN_MODULE_INIT(Init);

    static NAN_METHOD(New);

    static NAN_METHOD(route);
    static NAN_METHOD(nearest);
    static NAN_METHOD(table);
    static NAN_METHOD(tile);
    static NAN_METHOD(match);
    static NAN_METHOD(trip);

    Engine(osrm::EngineConfig &config);

    // Thread-safe singleton accessor
    static Nan::Persistent<v8::Function> &constructor();

    // Ref-counted OSRM alive even after shutdown until last callback is done
    std::shared_ptr<osrm::OSRM> this_;
};

} // ns node_osrm

NODE_MODULE(osrm, node_osrm::Engine::Init)

#endif
