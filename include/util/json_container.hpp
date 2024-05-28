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
 * Typed Value sum-type implemented as a variant able to represent tree-like JSON structures.
 *
 * Dispatch on its type by either by using apply_visitor or its get function.
 */
using Value = std::variant<String, Number, Object, Array, True, False, Null>;

/**
 * Typed Object.
 *
 * Unwrap the key-value pairs holding type via its values member attribute.
 */
struct Object
{
    std::unordered_map<std::string, Value> values;
};

/**
 * Typed Array.
 *
 * Unwrap the Value holding type via its values member attribute.
 */
struct Array
{
    std::vector<Value> values;
};

} // namespace osrm::util::json

#endif // JSON_CONTAINER_HPP
