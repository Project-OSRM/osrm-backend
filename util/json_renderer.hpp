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

#ifndef JSON_RENDERER_HPP
#define JSON_RENDERER_HPP

#include "cast.hpp"
#include "string_util.hpp"

#include <osrm/json_container.hpp>

namespace osrm
{
namespace json
{

struct Renderer : mapbox::util::static_visitor<>
{
    explicit Renderer(std::ostream &_out) : out(_out) {}

    void operator()(const String &string) const
    {
        out << "\"";
        out << escape_JSON(string.value);
        out << "\"";
    }

    void operator()(const Number &number) const
    {
        out.precision(10);
        out << number.value;
    }

    void operator()(const Object &object) const
    {
        out << "{";
        for (auto it = object.values.begin(), end = object.values.end(); it != end;)
        {
            out << "\"" << it->first << "\":";
            mapbox::util::apply_visitor(Renderer(out), it->second);
            if (++it != end)
            {
                out << ",";
            }
        }
        out << "}";
    }

    void operator()(const Array &array) const
    {
        out << "[";
        for (auto it = array.values.cbegin(), end = array.values.cend(); it != end;)
        {
            mapbox::util::apply_visitor(Renderer(out), *it);
            if (++it != end)
            {
                out << ",";
            }
        }
        out << "]";
    }

    void operator()(const True &) const { out << "true"; }

    void operator()(const False &) const { out << "false"; }

    void operator()(const Null &) const { out << "null"; }

  private:
    std::ostream &out;
};

struct ArrayRenderer : mapbox::util::static_visitor<>
{
    explicit ArrayRenderer(std::vector<char> &_out) : out(_out) {}

    void operator()(const String &string) const
    {
        out.push_back('\"');
        const auto string_to_insert = escape_JSON(string.value);
        out.insert(std::end(out), std::begin(string_to_insert), std::end(string_to_insert));
        out.push_back('\"');
    }

    void operator()(const Number &number) const
    {
        const std::string number_string = cast::to_string_with_precision(number.value);
        out.insert(out.end(), number_string.begin(), number_string.end());
    }

    void operator()(const Object &object) const
    {
        out.push_back('{');
        for (auto it = object.values.begin(), end = object.values.end(); it != end;)
        {
            out.push_back('\"');
            out.insert(out.end(), it->first.begin(), it->first.end());
            out.push_back('\"');
            out.push_back(':');

            mapbox::util::apply_visitor(ArrayRenderer(out), it->second);
            if (++it != end)
            {
                out.push_back(',');
            }
        }
        out.push_back('}');
    }

    void operator()(const Array &array) const
    {
        out.push_back('[');
        for (auto it = array.values.cbegin(), end = array.values.cend(); it != end;)
        {
            mapbox::util::apply_visitor(ArrayRenderer(out), *it);
            if (++it != end)
            {
                out.push_back(',');
            }
        }
        out.push_back(']');
    }

    void operator()(const True &) const
    {
        const std::string temp("true");
        out.insert(out.end(), temp.begin(), temp.end());
    }

    void operator()(const False &) const
    {
        const std::string temp("false");
        out.insert(out.end(), temp.begin(), temp.end());
    }

    void operator()(const Null &) const
    {
        const std::string temp("null");
        out.insert(out.end(), temp.begin(), temp.end());
    }

  private:
    std::vector<char> &out;
};

inline void render(std::ostream &out, const Object &object)
{
    Value value = object;
    mapbox::util::apply_visitor(Renderer(out), value);
}

inline void render(std::vector<char> &out, const Object &object)
{
    Value value = object;
    mapbox::util::apply_visitor(ArrayRenderer(out), value);
}

} // namespace json
} // namespace osrm
#endif // JSON_RENDERER_HPP
