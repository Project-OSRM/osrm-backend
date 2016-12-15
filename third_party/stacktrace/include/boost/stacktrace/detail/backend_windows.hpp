// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_DETAIL_BACKEND_WINDOWS_HPP
#define BOOST_STACKTRACE_DETAIL_BACKEND_WINDOWS_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <boost/core/noncopyable.hpp>
#include <boost/functional/hash.hpp>

#include <windows.h>
#include "Dbgeng.h"

#include <boost/detail/winapi/get_current_process.hpp>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Dbgeng.lib")

namespace boost { namespace stacktrace { namespace detail {

template <class T>
class com_holder: boost::noncopyable {
    T* holder_;

public:
    com_holder() BOOST_NOEXCEPT
        : holder_(0)
    {}

    T* operator->() const BOOST_NOEXCEPT {
        return holder_;
    }

    void** to_void_ptr_ptr() BOOST_NOEXCEPT {
        return reinterpret_cast<void**>(&holder_);
    }

    bool is_inited() const BOOST_NOEXCEPT {
        return !!holder_;
    }

    void reset() const BOOST_NOEXCEPT {
        if (holder_) {
            holder_->Release();
        }
    }

    ~com_holder() BOOST_NOEXCEPT {
        reset();
    }
};

inline bool try_init_com(com_holder<IDebugSymbols>& idebug_) BOOST_NOEXCEPT {
    if (idebug_.is_inited()) {
        return true;
    }

    CoInitializeEx(0, COINIT_MULTITHREADED);

    com_holder<IDebugClient> iclient;
    DebugCreate(__uuidof(IDebugClient), iclient.to_void_ptr_ptr());

    com_holder<IDebugControl> icontrol;
    iclient->QueryInterface(__uuidof(IDebugControl), icontrol.to_void_ptr_ptr());

    const bool res1 = (S_OK == iclient->AttachProcess(
        0,
        GetCurrentProcessId(),
        DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND)
    );
    if (!res1) {
        return false;
    }

    const bool res2 = (S_OK == icontrol->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE));
    if (!res2) {
        return false;
    }

    const bool res = (S_OK == iclient->QueryInterface(__uuidof(IDebugSymbols), idebug_.to_void_ptr_ptr()));
    if (!res) {
        idebug_.reset();
        return false;
    }

    return true;
}




backend::backend(void** memory, std::size_t size) BOOST_NOEXCEPT
    : hash_code_(0)
    , frames_count_(0)
    , data_(memory)
{
    boost::detail::winapi::ULONG_ hc = 0;
    frames_count_ = CaptureStackBackTrace(
        0,
        static_cast<boost::detail::winapi::ULONG_>(size),
        data_,
        &hc
    );

    boost::hash_combine(hash_code_, hc);
}

std::string backend::get_name(const void* addr) {
    std::string result;
    com_holder<IDebugSymbols> idebug_;
    if (!try_init_com(idebug_)) {
        return result;
    }
    const ULONG64 offset = reinterpret_cast<ULONG64>(addr);

    char name[256];
    name[0] = '\0';
    ULONG size = 0;
    bool res = (S_OK == idebug_->GetNameByOffset(
        offset,
        name,
        sizeof(name),
        &size,
        0
    ));

    if (!res && size != 0) {
        result.resize(size);
        res = (S_OK == idebug_->GetNameByOffset(
            offset,
            &result[0],
            static_cast<ULONG>(result.size()),
            &size,
            0
        ));
    } else if (res) {
        result = name;
    }

    if (!res) {
        result.clear();
    }

    return result;
}

std::string backend::get_source_file(const void* addr) {
    std::string result;
    com_holder<IDebugSymbols> idebug_;
    if (!try_init_com(idebug_)) {
        return result;
    }
    const ULONG64 offset = reinterpret_cast<ULONG64>(addr);

    char name[256];
    name[0] = 0;
    ULONG size = 0;
    bool res = (S_OK == idebug_->GetLineByOffset(
        offset,
        0,
        name,
        sizeof(name),
        &size,
        0
    ));

    if (!res && size != 0) {
        result.resize(size);
        res = (S_OK == idebug_->GetLineByOffset(
            offset,
            0,
            &result[0],
            static_cast<ULONG>(result.size()),
            &size,
            0
        ));
    } else if (res) {
        result = name;
    }


    if (!res) {
        result.clear();
    }

    return result;
}

std::size_t backend::get_source_line(const void* addr) {
    ULONG line_num = 0;

    com_holder<IDebugSymbols> idebug_;
    if (!try_init_com(idebug_)) {
        return 0;
    }

    const bool is_ok = (S_OK == idebug_->GetLineByOffset(
        reinterpret_cast<ULONG64>(addr),
        &line_num,
        0,
        0,
        0,
        0
    ));

    return (is_ok ? line_num : 0);
}


}}} // namespace boost::stacktrace::detail

#include <boost/stacktrace/detail/backend_common.ipp>

#endif // BOOST_STACKTRACE_DETAIL_BACKEND_LINUX_HPP
