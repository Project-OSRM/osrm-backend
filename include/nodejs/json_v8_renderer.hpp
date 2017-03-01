#ifndef OSRM_BINDINGS_NODE_JSON_V8_RENDERER_HPP
#define OSRM_BINDINGS_NODE_JSON_V8_RENDERER_HPP

#include "osrm/json_container.hpp"

#include <nan.h>

#include <functional>

namespace node_osrm
{

struct V8Renderer
{
    explicit V8Renderer(v8::Local<v8::Value> &_out) : out(_out) {}

    void operator()(const osrm::json::String &string) const
    {
        out = Nan::New(std::cref(string.value)).ToLocalChecked();
    }

    void operator()(const osrm::json::Number &number) const { out = Nan::New(number.value); }

    void operator()(const osrm::json::Object &object) const
    {
        v8::Local<v8::Object> obj = Nan::New<v8::Object>();
        for (const auto &keyValue : object.values)
        {
            v8::Local<v8::Value> child;
            mapbox::util::apply_visitor(V8Renderer(child), keyValue.second);
            obj->Set(Nan::New(keyValue.first).ToLocalChecked(), child);
        }
        out = obj;
    }

    void operator()(const osrm::json::Array &array) const
    {
        v8::Local<v8::Array> a = Nan::New<v8::Array>(array.values.size());
        for (auto i = 0u; i < array.values.size(); ++i)
        {
            v8::Local<v8::Value> child;
            mapbox::util::apply_visitor(V8Renderer(child), array.values[i]);
            a->Set(i, child);
        }
        out = a;
    }

    void operator()(const osrm::json::True &) const { out = Nan::New(true); }

    void operator()(const osrm::json::False &) const { out = Nan::New(false); }

    void operator()(const osrm::json::Null &) const { out = Nan::Null(); }

  private:
    v8::Local<v8::Value> &out;
};

inline void renderToV8(v8::Local<v8::Value> &out, const osrm::json::Object &object)
{
    osrm::json::Value value = object;
    mapbox::util::apply_visitor(V8Renderer(out), value);
}
}

#endif // JSON_V8_RENDERER_HPP
