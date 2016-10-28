
function usage() {
    echo "Usage:"
    echo ""
    echo "source ./scripts/setup_cpp11_toolchain.sh"
    echo ""
    exit 1
}


function main() {

    if [[ ${1:-unset} != "unset" ]]; then
      if [[ $1 == '-h' ]] || [[ $1 == '--help' ]]; then
        usage
      fi
    fi

    if [[ $(uname -s) == 'Linux' ]]; then
        set -e
        if [[ ! $(lsb_release --id) =~ "Ubuntu" ]]; then
            echo "only Ubuntu precise is supported at this time and not '$(lsb_release --id)'"
            exit 1
        fi

        local codename release
        codename=$(lsb_release --codename | cut -d : -f 2 | xargs basename)
        release=$(lsb_release --release | cut -d : -f 2 | xargs basename)

        export CPP11_TOOLCHAIN="$(pwd)/toolchain"
        mkdir -p ${CPP11_TOOLCHAIN}

        function dpack() {
            if [[ ! -f $2 ]]; then
                url=$1/$(echo $2 | sed 's/+/%2B/g')
                wget $url
                dpkg -x $2 ${CPP11_TOOLCHAIN}
            fi
        }

        local PPA LLVM_DIST
        PPA="https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test/+files"
        # http://llvm.org/apt/precise/dists/llvm-toolchain-${release}-3.5/main/binary-amd64/Packages
        # TODO: cache these for faster downloads
        LLVM_DIST="http://llvm.org/apt/${codename}/pool/main/l/llvm-toolchain-3.5"
        if [[ $codename == "precise" ]]; then
            dpack ${LLVM_DIST} clang-3.5_3.5~svn217304-1~exp1_amd64.deb &
            dpack ${LLVM_DIST} libllvm3.5_3.5~svn217304-1~exp1_amd64.deb &
            dpack ${LLVM_DIST} libclang-common-3.5-dev_3.5~svn215019-1~exp1_amd64.deb &
            dpack ${PPA} libstdc++6_4.8.1-2ubuntu1~${release}_amd64.deb &
            dpack ${PPA} libstdc++-4.8-dev_4.8.1-2ubuntu1~${release}_amd64.deb &
            dpack ${PPA} libgcc-4.8-dev_4.8.1-2ubuntu1~${release}_amd64.deb &
            wait
        elif [[ $codename == "trusty" ]]; then
            dpack ${LLVM_DIST} clang-3.5_3.5~svn215019-1~exp1_amd64.deb &
            dpack ${LLVM_DIST} libllvm3.5_3.5~svn215019-1~exp1_amd64.deb &
            dpack ${LLVM_DIST} libclang-common-3.5-dev_3.5.1~svn225255-1~exp1_amd64.deb &
            wait
        else
            echo "unsupported distro: $codename $release"
            exit 1
        fi
        export CPLUS_INCLUDE_PATH="${CPP11_TOOLCHAIN}/usr/include/c++/4.8:${CPP11_TOOLCHAIN}/usr/include/x86_64-linux-gnu/c++/4.8:${CPLUS_INCLUDE_PATH:-}"
        export LD_LIBRARY_PATH="${CPP11_TOOLCHAIN}/usr/lib/x86_64-linux-gnu:${CPP11_TOOLCHAIN}/usr/lib/gcc/x86_64-linux-gnu/4.8/:${LD_LIBRARY_PATH:-}"
        export LIBRARY_PATH="${LD_LIBRARY_PATH}:${LIBRARY_PATH:-}"
        export PATH="${CPP11_TOOLCHAIN}/usr/bin":${PATH}
        export CXX="${CPP11_TOOLCHAIN}/usr/bin/clang++-3.5"
        export CC="${CPP11_TOOLCHAIN}/usr/bin/clang-3.5"
        set +e
    else
        echo "Nothing to be done: this script only bootstraps a c++11 toolchain for linux"
    fi
}

main $@