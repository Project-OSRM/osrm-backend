#ifndef OSRM_BINDINGS_NODE_JSON_V8_RENDERER_HPP
#define OSRM_BINDINGS_NODE_JSON_V8_RENDERER_HPP

#include "osrm/json_container.hpp"
#include <napi.h>

#include <functional>

namespace node_osrm
{

struct V8Renderer
{
    explicit V8Renderer(const Napi::Env &env) : env(env) {}

    Napi::Value operator()(const osrm::json::String &string) const
    {
        return Napi::String::New(env, string.value);
    }

    Napi::Value operator()(const osrm::json::Number &number) const
    {
        return Napi::Number::New(env, number.value);
    }

    Napi::Value operator()(const osrm::json::Object &object) const
    {
        Napi::Object obj = Napi::Object::New(env);
        for (const auto &keyValue : object.values)
        {
            obj.Set(keyValue.first, visit(*this, keyValue.second));
        }
        return obj;
    }

    Napi::Value operator()(const osrm::json::Array &array) const
    {
        Napi::Array a = Napi::Array::New(env, array.values.size());
        for (auto i = 0u; i < array.values.size(); ++i)
        {
            a.Set(i, visit(*this, array.values[i]));
        }
        return a;
    }

    Napi::Value operator()(const osrm::json::True &) const { return Napi::Boolean::New(env, true); }

    Napi::Value operator()(const osrm::json::False &) const
    {
        return Napi::Boolean::New(env, false);
    }

    Napi::Value operator()(const osrm::json::Null &) const { return env.Null(); }

  private:
    Napi::Value visit(const V8Renderer &renderer, const osrm::json::Value &value) const
    {
        switch (value.index())
        {
        case 0:
            return renderer(std::get<osrm::json::String>(value));
            break;
        case 1:
            return renderer(std::get<osrm::json::Number>(value));
            break;
        case 2:
            return renderer(std::get<osrm::json::Object>(value));
            break;
        case 3:
            return renderer(std::get<osrm::json::Array>(value));
            break;
        case 4:
            return renderer(std::get<osrm::json::True>(value));
            break;
        case 5:
            return renderer(std::get<osrm::json::False>(value));
            break;
        case 6:
            return renderer(std::get<osrm::json::Null>(value));
            break;
        }
        return env.Null();
    }

  private:
    const Napi::Env &env;
};

inline void renderToV8(const Napi::Env &env, Napi::Value &out, const osrm::json::Object &object)
{
    V8Renderer renderer(env);
    out = renderer(object);
}
} // namespace node_osrm

#endif // JSON_V8_RENDERER_HPP
