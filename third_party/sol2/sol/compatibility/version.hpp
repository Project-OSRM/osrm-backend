// sol2 

// The MIT License (MIT)

// Copyright (c) 2013-2018 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SOL_VERSION_HPP
#define SOL_VERSION_HPP

#include "../feature_test.hpp"

#if defined(SOL_USING_CXX_LUA) && SOL_USING_CXX_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#if defined(SOL_USING_CXX_LUAJIT) && SOL_USING_CXX_LUAJIT
#include <luajit.h>
#endif // C++ LuaJIT ... whatever that means
#if (!defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) || !(SOL_EXCEPTIONS_SAFE_PROPAGATION)) && (!defined(SOL_EXCEPTIONS_ALWAYS_UNSAFE) || !(SOL_EXCEPTIONS_ALWAYS_UNSAFE))
#define SOL_EXCEPTIONS_SAFE_PROPAGATION 1
#endif // Exceptions can be propagated safely using C++-compiled Lua
#else
#include <lua.hpp>
#endif // C++ Mangling for Lua

#ifdef LUAJIT_VERSION
#ifndef SOL_LUAJIT
#define SOL_LUAJIT 1
#ifndef SOL_LUAJIT_VERSION
#define SOL_LUAJIT_VERSION LUAJIT_VERSION_NUM
#endif // SOL_LUAJIT_VERSION definition, if not present
#endif // sol luajit
#endif // luajit

#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 502
#define SOL_LUA_VERSION LUA_VERSION_NUM
#elif defined(LUA_VERSION_NUM) && LUA_VERSION_NUM == 501
#define SOL_LUA_VERSION LUA_VERSION_NUM
#elif !defined(LUA_VERSION_NUM) || !(LUA_VERSION_NUM)
// Definitely 5.0
#define SOL_LUA_VERSION 500
#else
// ??? Not sure, assume 502?
#define SOL_LUA_VERSION 502
#endif // Lua Version 502, 501 || luajit, 500

#endif // SOL_VERSION_HPP
