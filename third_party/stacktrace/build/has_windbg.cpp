// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <cstring>
#include <windows.h>
#include "Dbgeng.h"
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Dbgeng.lib")

int main() {
    CoInitializeEx(0, COINIT_MULTITHREADED);
}
