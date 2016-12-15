// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <dlfcn.h>
#include <execinfo.h>

int main() {
    void* buffer[10];
    ::backtrace(buffer, 10);

}
