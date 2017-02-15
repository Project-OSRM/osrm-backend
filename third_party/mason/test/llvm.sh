#!/usr/bin/env bash

set -e -u
set -o pipefail

failure=0

if [[ $(uname -s) == 'Linux' ]]; then

    # install both llvm variants
    ./mason install llvm 3.8.1            # linked to libc++
    ./mason install llvm 3.8.1-libstdcxx  # linked to libstd++
    LLVM1=$(./mason prefix llvm 3.8.1)
    LLVM2=$(./mason prefix llvm 3.8.1-libstdcxx)

    if [[ ${LLVM1} == ${LLVM2} ]]; then
        echo "expected prefix to be different for both llvm versions"
        failure=1
    fi

    CXXFLAGS1=$($(./mason prefix llvm 3.8.1)/bin/llvm-config --cxxflags)
    if [[ ${CXXFLAGS1} =~ '-stdlib=libc++' ]]; then
        # found expected libc++ flag
        :
    else
        echo "Did not find libc++ in flags"
        failure=1
    fi

    CXXFLAGS2=$($(./mason prefix llvm 3.8.1-libstdcxx)/bin/llvm-config --cxxflags)
    if [[ ${CXXFLAGS2} =~ '-stdlib=libc++' ]]; then
        echo "Found libc++ in flags (unexpected for libstdc++ package variant)"
        failure=1
    fi
fi

exit $failure