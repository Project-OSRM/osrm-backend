#ifndef OSRM_BINDINGS_NODE_JSON_V8_RENDERER_HPP
#define OSRM_BINDINGS_NODE_JSON_V8_RENDERER_HPP

#include "osrm/json_container.hpp"
#include <napi.h>

#include <functional>

namespace node_osrm
{

struct V8Renderer
{
    explicit V8Renderer(const Napi::Env &env, Napi::Value &out) : env(env), out(out) {}

    void operator()(const osrm::json::String &string) const
    {
        out = Napi::String::New(env, string.value);
    }

    void operator()(const osrm::json::Number &number) const
    {
        out = Napi::Number::New(env, number.value);
    }

    void operator()(const osrm::json::Object &object) const
    {
        Napi::Object obj = Napi::Object::New(env);
        for (const auto &keyValue : object.values)
        {
            Napi::Value child;
            std::visit(V8Renderer(env, child), keyValue.second);
            obj.Set(keyValue.first, child);
        }
        out = obj;
    }

    void operator()(const osrm::json::Array &array) const
    {
        Napi::Array a = Napi::Array::New(env, array.values.size());
        for (auto i = 0u; i < array.values.size(); ++i)
        {
            Napi::Value child;
            std::visit(V8Renderer(env, child), array.values[i]);
            a.Set(i, child);
        }
        out = a;
    }

    void operator()(const osrm::json::True &) const { out = Napi::Boolean::New(env, true); }

    void operator()(const osrm::json::False &) const { out = Napi::Boolean::New(env, false); }

    void operator()(const osrm::json::Null &) const { out = env.Null(); }

  private:
    const Napi::Env &env;
    Napi::Value &out;
};

inline void renderToV8(const Napi::Env &env, Napi::Value &out, const osrm::json::Object &object)
{
    V8Renderer renderer(env, out);
    renderer(object);
}
} // namespace node_osrm

#endif // JSON_V8_RENDERER_HPP
