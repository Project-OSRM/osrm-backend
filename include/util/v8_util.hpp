#ifndef V8_UTIL_HPP
#define V8_UTIL_HPP

#include <libplatform/libplatform.h>
#include <v8.h>

#include <iostream>
#include <string>

namespace osrm
{
namespace util
{

class V8Allocator : public v8::ArrayBuffer::Allocator
{
  public:
    virtual void *Allocate(size_t length)
    {
        void *data = AllocateUninitialized(length);
        return data == NULL ? data : memset(data, 0, length);
    }
    virtual void *AllocateUninitialized(size_t length) { return malloc(length); }
    virtual void Free(void *data, size_t) { free(data); }
};

class V8Env
{
  public:
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Object> new_object() { return v8::Object::New(isolate); }

    template <typename T>
    v8::Local<v8::Object> new_object(v8::Local<v8::Function> constructor, T &wrapped)
    {
        v8::Local<v8::Object> obj = constructor->NewInstance();
        obj->SetInternalField(0, v8::External::New(isolate, &wrapped));
        return obj;
    }

    v8::Local<v8::String> new_string(const char *str, int length = -1)
    {
        return v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kNormal, length)
            .ToLocalChecked();
    }

    v8::Local<v8::String> new_string(const std::string& str)
    {
        return new_string(str.data(), str.size());
    }

    v8::Local<v8::Integer> new_integer(const int32_t num) { return v8::Integer::New(isolate, num); }

    v8::Local<v8::Number> new_number(const double num) { return v8::Number::New(isolate, num); }

    v8::Local<v8::FunctionTemplate>
    new_function_template(v8::FunctionCallback callback = 0,
                          v8::Local<v8::Value> data = v8::Local<v8::Value>(),
                          v8::Local<v8::Signature> signature = v8::Local<v8::Signature>(),
                          int length = 0)
    {
        return v8::FunctionTemplate::New(isolate, callback, data, signature, length);
    }

    void set(v8::Local<v8::Object> obj, const char *str, v8::Local<v8::Value> val)
    {
        (void)obj->Set(context, new_string(str), val);
    }

    void set_property(v8::Local<v8::Object> obj,
                      const char *str,
                      v8::Local<v8::Value> val,
                      v8::PropertyAttribute attribs = v8::PropertyAttribute::None)
    {
        (void)obj->DefineOwnProperty(context, new_string(str), val, attribs);
    }


    v8::Local<v8::Value> get(v8::Local<v8::Object> obj, const char *str)
    {
        v8::Local<v8::Value> value;
        (void)obj->Get(context, new_string(str)).ToLocal(&value);
        return value;
    }

    double get_number(v8::Local<v8::Object> obj, const char *str, double default_value = 0)
    {
        auto value = obj->Get(context, new_string(str));
        if (!value.IsEmpty())
        {
            return value.ToLocalChecked().As<v8::Number>()->Value();
        }
        else
        {
            return default_value;
        }
    }

    std::string
    get_string(v8::Local<v8::Object> obj, const uint32_t idx, const std::string &default_value = "")
    {
        auto value = obj->Get(context, idx);
        if (!value.IsEmpty() && !value.ToLocalChecked()->IsUndefined())
        {
            v8::String::Utf8Value utf8_value(value.ToLocalChecked()->ToString());
            return {*utf8_value, static_cast<size_t>(utf8_value.length())};
        }
        else
        {
            return default_value;
        }
    }

    void freeze(v8::Local<v8::Value> obj)
    {
        v8::Local<v8::Object> object = get(context->Global(), "Object").As<v8::Object>();
        get(object, "freeze").As<v8::Function>()->Call(object, 1, &obj);
    }
};

class V8Object
{
  public:
    V8Object(const V8Env &env_, const char *name) : env(env_)
    {
        tpl = env.new_function_template();
        tpl->SetClassName(env.new_string(name));
        itpl = tpl->InstanceTemplate();
        itpl->SetInternalFieldCount(1);
    }

    void method(const char *name, v8::FunctionCallback callback)
    {
        v8::Local<v8::FunctionTemplate> t = env.new_function_template(
            callback, v8::Local<v8::Value>(), v8::Signature::New(env.isolate, tpl));
        auto fn_name = env.new_string(name);
        tpl->PrototypeTemplate()->Set(fn_name, t);
        t->SetClassName(fn_name);
    }

    void property(const char *name, v8::AccessorGetterCallback getter)
    {
        itpl->SetAccessor(env.new_string(name),
                          getter,
                          0,
                          v8::Local<v8::Value>(),
                          v8::AccessControl::DEFAULT,
                          static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly |
                                                             v8::PropertyAttribute::DontDelete));
    }

    void
    property(const char *name, v8::AccessorGetterCallback getter, v8::AccessorSetterCallback setter)
    {
        itpl->SetAccessor(env.new_string(name),
                          getter,
                          setter,
                          v8::Local<v8::Value>(),
                          v8::AccessControl::DEFAULT,
                          v8::PropertyAttribute::DontDelete);
    }

    v8::Local<v8::Function> get() { return tpl->GetFunction(); }

  private:
    V8Env env;
    v8::Local<v8::FunctionTemplate> tpl;
    v8::Local<v8::ObjectTemplate> itpl;
};

template <typename T> struct V8Class
{
    template <typename U> static T &Unwrap(const U &info)
    {
        v8::Local<v8::Object> self = info.Holder();
        v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
        return *static_cast<T *>(wrap->Value());
    }
};
}
}

#endif // V8_UTIL_HPP
