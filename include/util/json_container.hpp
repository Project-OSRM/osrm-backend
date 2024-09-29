/*

Copyright (c) 2017, Project OSRM contributors
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

#ifndef JSON_CONTAINER_HPP
#define JSON_CONTAINER_HPP

#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace osrm::util::json
{

// fwd. decls.
struct Object;
struct Array;

/**
 * Typed string wrapper.
 *
 * Unwrap the type via its value member attribute.
 */
struct String
{
    String() = default;
    String(const char *value_) : value{value_} {}
    String(std::string value_) : value{std::move(value_)} {}
    std::string value;
};

/**
 * Typed floating point number.
 *
 * Unwrap the type via its value member attribute.
 */
struct Number
{
    Number() = default;
    Number(double value_) : value{value_} {}
    double value;
};

/**
 * Typed True.
 */
struct True
{
};

/**
 * Typed False.
 */
struct False
{
};

/**
 * Typed Null.
 */
struct Null
{
};

/**
 * Typed Object.
 *
 * Unwrap the key-value pairs holding type via its values member attribute.
 */
struct Object;

/**
 * Typed Array.
 *
 * Unwrap the Value holding type via its values member attribute.
 */
struct Array;

struct Value;

// Definitions of Object and Array (must come after Value is defined)
struct Object
{
    std::unordered_map<std::string, Value> values;
};

struct Array
{
    std::vector<Value> values;
};

struct Value
{
    enum class Type
    {
        Invalid,
        String,
        Number,
        Object,
        Array,
        True,
        False,
        Null
    };
    String string;
    Number number;
    Object object;
    Array array;
    Type type;

    Value() noexcept : type(Type::Invalid) {}
    Value(const Null &) noexcept : type(Type::Null) {}
    Value(const True &) noexcept : type(Type::True) {}
    Value(const False &) noexcept : type(Type::False) {}
    Value(String &&string_) noexcept : string(std::move(string_)), type(Type::String) {}
    Value(Number &&number_) noexcept : number(number_), type(Type::Number) {}
    Value(Object &&object_) noexcept : object(std::move(object_)), type(Type::Object) {}
    Value(Array &&array_) noexcept : array(std::move(array_)), type(Type::Array) {}
    Value(const String &string_) noexcept : string(string_), type(Type::String) {}
    Value(const Number &number_) noexcept : number(number_), type(Type::Number) {}
    Value(const Object &object_) noexcept : object(object_), type(Type::Object) {}
    Value(const Array &array_) noexcept : array(array_), type(Type::Array) {}

    Value(double number) noexcept : number(number), type(Type::Number) {}
    Value(std::string string) noexcept : string(std::move(string)), type(Type::String) {}
    Value(const char *string) noexcept : string(string), type(Type::String) {}
};

} // namespace osrm::util::json

namespace std
{
template <typename T> inline T &get(osrm::util::json::Value &value) noexcept;

template <>
inline osrm::util::json::String &
get<osrm::util::json::String>(osrm::util::json::Value &value) noexcept
{
    return value.string;
}

template <>
inline osrm::util::json::Number &
get<osrm::util::json::Number>(osrm::util::json::Value &value) noexcept
{
    return value.number;
}

template <>
inline osrm::util::json::Object &
get<osrm::util::json::Object>(osrm::util::json::Value &value) noexcept
{
    return value.object;
}

template <>
inline osrm::util::json::Array &
get<osrm::util::json::Array>(osrm::util::json::Value &value) noexcept
{
    return value.array;
}

template <typename T> inline const T &get(const osrm::util::json::Value &value) noexcept;

template <>
inline const osrm::util::json::String &
get<osrm::util::json::String>(const osrm::util::json::Value &value) noexcept
{
    return value.string;
}

template <>
inline const osrm::util::json::Number &
get<osrm::util::json::Number>(const osrm::util::json::Value &value) noexcept
{
    return value.number;
}

template <>
inline const osrm::util::json::Object &
get<osrm::util::json::Object>(const osrm::util::json::Value &value) noexcept
{
    return value.object;
}

template <>
inline const osrm::util::json::Array &
get<osrm::util::json::Array>(const osrm::util::json::Value &value) noexcept
{
    return value.array;
}

template <typename Visitor>
inline void visit(Visitor &&visitor, const osrm::util::json::Value &value)
{
    switch (value.type)
    {
    case osrm::util::json::Value::Type::String:
        visitor(value.string);
        break;
    case osrm::util::json::Value::Type::Number:
        visitor(value.number);
        break;
    case osrm::util::json::Value::Type::Object:
        visitor(value.object);
        break;
    case osrm::util::json::Value::Type::Array:
        visitor(value.array);
        break;
    case osrm::util::json::Value::Type::True:
        visitor(osrm::util::json::True{});
        break;
    case osrm::util::json::Value::Type::False:
        visitor(osrm::util::json::False{});
        break;
    case osrm::util::json::Value::Type::Null:
        visitor(osrm::util::json::Null{});
        break;
    case osrm::util::json::Value::Type::Invalid:
        break;
    }
}

} // namespace std
#endif // JSON_CONTAINER_HPP
