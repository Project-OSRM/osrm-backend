/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

// based on
// https://svn.apache.org/repos/asf/mesos/tags/release-0.9.0-incubating-RC0/src/common/json.hpp

#ifndef JSON_V8_RENDERER_HPP
#define JSON_V8_RENDERER_HPP

#include <osrm/json_container.hpp>

// v8
#include <nan.h>

#include <functional>

namespace node_osrm
{

struct V8Renderer : mapbox::util::static_visitor<>
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
    // FIXME this should be a cast?
    osrm::json::Value value = object;
    mapbox::util::apply_visitor(V8Renderer(out), value);
}

}

#endif // JSON_V8_RENDERER_HPP
