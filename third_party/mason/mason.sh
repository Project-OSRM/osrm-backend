set -e
set -o pipefail
# set -x

export MASON_ROOT=${MASON_ROOT:-`pwd`/mason_packages}
MASON_BUCKET=${MASON_BUCKET:-mason-binaries}
MASON_IGNORE_OSX_SDK=${MASON_IGNORE_OSX_SDK:-false}

MASON_UNAME=`uname -s`
if [ ${MASON_UNAME} = 'Darwin' ]; then
    MASON_PLATFORM=${MASON_PLATFORM:-osx}
    MASON_XCODE_ROOT=`"xcode-select" -p`
elif [ ${MASON_UNAME} = 'Linux' ]; then
    MASON_PLATFORM=${MASON_PLATFORM:-linux}
fi

# In non-interactive environments like Travis CI, we can't use -s because it'll fill up the log
# way too fast
case $- in
    *i*) MASON_CURL_ARGS=   ;; # interactive
    *)   MASON_CURL_ARGS=-s ;; # non-interative
esac

case ${MASON_UNAME} in
    'Darwin')    MASON_CONCURRENCY=`sysctl -n hw.ncpu` ;;
    'Linux')        MASON_CONCURRENCY=$(lscpu -p | egrep -v '^#' | wc -l) ;;
    *)              MASON_CONCURRENCY=1 ;;
esac


function mason_step    { >&2 echo -e "\033[1m\033[36m* $1\033[0m"; }
function mason_substep { >&2 echo -e "\033[1m\033[36m* $1\033[0m"; }
function mason_success { >&2 echo -e "\033[1m\033[32m* $1\033[0m"; }
function mason_error   { >&2 echo -e "\033[1m\033[31m$1\033[0m"; }


case ${MASON_ROOT} in
    *\ * ) mason_error "Directory '${MASON_ROOT} contains spaces."; exit ;;
esac

if [ ${MASON_PLATFORM} = 'osx' ]; then
    export MASON_HOST_ARG="--host=x86_64-apple-darwin"
    export MASON_PLATFORM_VERSION=`uname -m`

    if [[ ${MASON_IGNORE_OSX_SDK} == false ]]; then
        MASON_SDK_VERSION=`xcrun --sdk macosx --show-sdk-version`
        MASON_SDK_ROOT=${MASON_XCODE_ROOT}/Platforms/MacOSX.platform/Developer
        MASON_SDK_PATH="${MASON_SDK_ROOT}/SDKs/MacOSX${MASON_SDK_VERSION}.sdk"

        if [[ ${MASON_SYSTEM_PACKAGE} && ${MASON_SDK_VERSION%%.*} -ge 10 && ${MASON_SDK_VERSION##*.} -ge 11 ]]; then
            export MASON_DYNLIB_SUFFIX="tbd"
        else
            export MASON_DYNLIB_SUFFIX="dylib"
        fi

        MIN_SDK_VERSION_FLAG="-mmacosx-version-min=10.8"
        SYSROOT_FLAGS="-isysroot ${MASON_SDK_PATH} -arch x86_64 ${MIN_SDK_VERSION_FLAG}"
        export CFLAGS="${SYSROOT_FLAGS}"
        export CXXFLAGS="${CFLAGS} -fvisibility-inlines-hidden -stdlib=libc++ -std=c++11"
        # NOTE: OSX needs '-stdlib=libc++ -std=c++11' in both CXXFLAGS and LDFLAGS
        # to correctly target c++11 for build systems that don't know about it yet (like libgeos 3.4.2)
        # But because LDFLAGS is also for C libs we can only put these flags into LDFLAGS per package
        export LDFLAGS="-Wl,-search_paths_first ${SYSROOT_FLAGS}"
        export CXX="/usr/bin/clang++"
        export CC="/usr/bin/clang"
    fi

elif [ ${MASON_PLATFORM} = 'ios' ]; then
    export MASON_HOST_ARG="--host=arm-apple-darwin"
    export MASON_PLATFORM_VERSION="8.0" # Deployment target version

    MASON_SDK_VERSION=`xcrun --sdk iphoneos --show-sdk-version`
    MASON_SDK_ROOT=${MASON_XCODE_ROOT}/Platforms/iPhoneOS.platform/Developer
    MASON_SDK_PATH="${MASON_SDK_ROOT}/SDKs/iPhoneOS${MASON_SDK_VERSION}.sdk"

    MIN_SDK_VERSION_FLAG="-miphoneos-version-min=${MASON_PLATFORM_VERSION}"
    export MASON_IOS_CFLAGS="${MIN_SDK_VERSION_FLAG} -isysroot ${MASON_SDK_PATH}"
    if [[ ${MASON_SDK_VERSION%%.*} -ge 9 ]]; then
        export MASON_IOS_CFLAGS="${MASON_IOS_CFLAGS} -fembed-bitcode"
        export MASON_DYNLIB_SUFFIX="tbd"
    else
        export MASON_DYNLIB_SUFFIX="dylib"
    fi

    if [ `xcrun --sdk iphonesimulator --show-sdk-version` != ${MASON_SDK_VERSION} ]; then
        mason_error "iPhone Simulator SDK version doesn't match iPhone SDK version"
        exit 1
    fi

    MASON_SDK_ROOT=${MASON_XCODE_ROOT}/Platforms/iPhoneSimulator.platform/Developer
    MASON_SDK_PATH="${MASON_SDK_ROOT}/SDKs/iPhoneSimulator${MASON_SDK_VERSION}.sdk"
    export MASON_ISIM_CFLAGS="${MIN_SDK_VERSION_FLAG} -isysroot ${MASON_SDK_PATH}"

elif [ ${MASON_PLATFORM} = 'linux' ]; then

    export MASON_DYNLIB_SUFFIX="so"

    # Assume current system is the target platform
    if [ -z ${MASON_PLATFORM_VERSION} ] ; then
        export MASON_PLATFORM_VERSION=`uname -m`
    fi

    export CFLAGS="-fPIC"
    export CXXFLAGS="${CFLAGS} -std=c++11"

    if [ `uname -m` != ${MASON_PLATFORM_VERSION} ] ; then
        # Install the cross compiler
        MASON_XC_PACKAGE_NAME=gcc
        MASON_XC_PACKAGE_VERSION=${MASON_XC_GCC_VERSION:-5.3.0}-${MASON_PLATFORM_VERSION}
        MASON_XC_PACKAGE=${MASON_XC_PACKAGE_NAME}-${MASON_XC_PACKAGE_VERSION}
        MASON_XC_ROOT=$(MASON_PLATFORM= MASON_PLATFORM_VERSION= ${MASON_DIR}/mason prefix ${MASON_XC_PACKAGE_NAME} ${MASON_XC_PACKAGE_VERSION})
        if [[ ! ${MASON_XC_ROOT} =~ ".build" ]] && [ ! -d ${MASON_XC_ROOT} ] ; then
            MASON_PLATFORM= MASON_PLATFORM_VERSION= ${MASON_DIR}/mason install ${MASON_XC_PACKAGE_NAME} ${MASON_XC_PACKAGE_VERSION}
            MASON_XC_ROOT=$(MASON_PLATFORM= MASON_PLATFORM_VERSION= ${MASON_DIR}/mason prefix ${MASON_XC_PACKAGE_NAME} ${MASON_XC_PACKAGE_VERSION})
        fi

        # Load toolchain specific variables
        if [[ ! ${MASON_XC_ROOT} =~ ".build" ]] && [ -f ${MASON_XC_ROOT}/toolchain.sh ] ; then
            source ${MASON_XC_ROOT}/toolchain.sh
        fi
    fi

elif [ ${MASON_PLATFORM} = 'android' ]; then
    case "${MASON_PLATFORM_VERSION}" in
        arm-v5-9) export MASON_ANDROID_ABI=arm-v5 ;;
        arm-v7-9) export MASON_ANDROID_ABI=arm-v7 ;;
        arm-v8-21) export MASON_ANDROID_ABI=arm-v8 ;;
        x86-9) export MASON_ANDROID_ABI=x86 ;;
        x86-64-21) export MASON_ANDROID_ABI=x86-64 ;;
        mips-9) export MASON_ANDROID_ABI=mips ;;
        mips64-21) export MASON_ANDROID_ABI=mips64 ;;
        *) export MASON_ANDROID_ABI=${MASON_ANDROID_ABI:-arm-v7}
    esac

    CFLAGS="-g -DANDROID -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -Wa,--noexecstack -Wformat -Werror=format-security"
    LDFLAGS="-Wl,--build-id -Wl,--warn-shared-textrel -Wl,--fatal-warnings -Wl,--no-undefined -Wl,-z,noexecstack -Qunused-arguments -Wl,-z,relro -Wl,-z,now"
    export CPPFLAGS="-D__ANDROID__"

    if [ ${MASON_ANDROID_ABI} = 'arm-v8' ]; then
        MASON_ANDROID_TOOLCHAIN="aarch64-linux-android"
        export MASON_HOST_ARG="--host=${MASON_ANDROID_TOOLCHAIN}"

        CFLAGS="-target aarch64-none-linux-android ${CFLAGS}"

        export JNIDIR="arm64-v8a"
        MASON_ANDROID_ARCH="arm64"
        MASON_ANDROID_PLATFORM="21"

    elif [ ${MASON_ANDROID_ABI} = 'arm-v7' ]; then
        MASON_ANDROID_TOOLCHAIN="arm-linux-androideabi"
        export MASON_HOST_ARG="--host=${MASON_ANDROID_TOOLCHAIN}"

        CFLAGS="-target armv7-none-linux-androideabi ${CFLAGS} -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -fno-integrated-as -mthumb"
        LDFLAGS="${LDFLAGS} -Wl,--fix-cortex-a8 -Wl,--exclude-libs,libunwind.a"

        export JNIDIR="armeabi-v7a"
        MASON_ANDROID_ARCH="arm"
        MASON_ANDROID_PLATFORM="9"

    elif [ ${MASON_ANDROID_ABI} = 'arm-v5' ]; then
        MASON_ANDROID_TOOLCHAIN="arm-linux-androideabi"
        export MASON_HOST_ARG="--host=${MASON_ANDROID_TOOLCHAIN}"

        CFLAGS="-target armv5te-none-linux-androideabi ${CFLAGS} -march=armv5te -mtune=xscale -msoft-float -fno-integrated-as -mthumb"
        LDFLAGS="${LDFLAGS} -Wl,--exclude-libs,libunwind.a"

        export JNIDIR="armeabi"
        MASON_ANDROID_ARCH="arm"
        MASON_ANDROID_PLATFORM="9"

    elif [ ${MASON_ANDROID_ABI} = 'x86' ]; then
        MASON_ANDROID_TOOLCHAIN="i686-linux-android"
        export MASON_HOST_ARG="--host=${MASON_ANDROID_TOOLCHAIN}"

        CFLAGS="-target i686-none-linux-android ${CFLAGS}"

        export JNIDIR="x86"
        MASON_ANDROID_ARCH="x86"
        MASON_ANDROID_PLATFORM="9"

    elif [ ${MASON_ANDROID_ABI} = 'x86-64' ]; then
        MASON_ANDROID_TOOLCHAIN="x86_64-linux-android"
        export MASON_HOST_ARG="--host=${MASON_ANDROID_TOOLCHAIN}"

        export JNIDIR="x86_64"
        CFLAGS="-target x86_64-none-linux-android ${CFLAGS}"

        MASON_ANDROID_ARCH="x86_64"
        MASON_ANDROID_PLATFORM="21"

    elif [ ${MASON_ANDROID_ABI} = 'mips' ]; then
        MASON_ANDROID_TOOLCHAIN="mipsel-linux-android"
        export MASON_HOST_ARG="--host=${MASON_ANDROID_TOOLCHAIN}"

        CFLAGS="-target mipsel-none-linux-android ${CFLAGS} -mips32"

        export JNIDIR="mips"
        MASON_ANDROID_ARCH="mips"
        MASON_ANDROID_PLATFORM="9"

    elif [ ${MASON_ANDROID_ABI} = 'mips-64' ]; then
        MASON_ANDROID_TOOLCHAIN="mips64el-linux-android"
        export MASON_HOST_ARG="--host=${MASON_ANDROID_TOOLCHAIN}"

        CFLAGS="-target mips64el-none-linux-android ${CFLAGS}"

        export JNIDIR="mips64"
        MASON_ANDROID_ARCH="mips64"
        MASON_ANDROID_PLATFORM="21"
    fi

    export MASON_DYNLIB_SUFFIX="so"
    export MASON_PLATFORM_VERSION="${MASON_ANDROID_ABI}-${MASON_ANDROID_PLATFORM}"
    MASON_API_LEVEL=${MASON_API_LEVEL:-android-$MASON_ANDROID_PLATFORM}

    # Installs the native SDK
    export MASON_NDK_PACKAGE_VERSION=${MASON_ANDROID_ARCH}-${MASON_ANDROID_PLATFORM}-r13b
    MASON_SDK_ROOT=$(MASON_PLATFORM= MASON_PLATFORM_VERSION= ${MASON_DIR}/mason prefix android-ndk ${MASON_NDK_PACKAGE_VERSION})
    if [ ! -d ${MASON_SDK_ROOT} ] ; then
        MASON_PLATFORM= MASON_PLATFORM_VERSION= ${MASON_DIR}/mason install android-ndk ${MASON_NDK_PACKAGE_VERSION}
    fi
    MASON_SDK_PATH="${MASON_SDK_ROOT}/sysroot"
    export PATH=${MASON_SDK_ROOT}/bin:${PATH}

    export CFLAGS="--sysroot=${MASON_SDK_PATH} ${CFLAGS}"
    export CXXFLAGS="--sysroot=${MASON_SDK_PATH} ${CFLAGS}"
    export LDFLAGS="--sysroot=${MASON_SDK_PATH} ${LDFLAGS}"

    export CXX="${MASON_ANDROID_TOOLCHAIN}-clang++"
    export CC="${MASON_ANDROID_TOOLCHAIN}-clang"
    export LD="${MASON_ANDROID_TOOLCHAIN}-ld"
    export AR="${MASON_ANDROID_TOOLCHAIN}-ar"
    export RANLIB="${MASON_ANDROID_TOOLCHAIN}-ranlib"
    export STRIP="${MASON_ANDROID_TOOLCHAIN}-strip"
fi


# Variable defaults
MASON_HOST_ARG=${MASON_HOST_ARG:-}
MASON_PLATFORM_VERSION=${MASON_PLATFORM_VERSION:-0}
MASON_NAME=${MASON_NAME:-nopackage}
MASON_VERSION=${MASON_VERSION:-noversion}
MASON_HEADER_ONLY=${MASON_HEADER_ONLY:-false}
MASON_SLUG=${MASON_NAME}-${MASON_VERSION}
if [[ ${MASON_HEADER_ONLY} == true ]]; then
    MASON_PLATFORM_ID=headers
else
    MASON_PLATFORM_ID=${MASON_PLATFORM}-${MASON_PLATFORM_VERSION}
fi
MASON_PREFIX=${MASON_ROOT}/${MASON_PLATFORM_ID}/${MASON_NAME}/${MASON_VERSION}
MASON_BINARIES=${MASON_PLATFORM_ID}/${MASON_NAME}/${MASON_VERSION}.tar.gz
MASON_BINARIES_PATH=${MASON_ROOT}/.binaries/${MASON_BINARIES}




function mason_check_existing {
    # skip installing if it already exists
    if [ ${MASON_HEADER_ONLY:-false} = true ] ; then
        if [ -d "${MASON_PREFIX}" ] ; then
            mason_success "Already installed at ${MASON_PREFIX}"
            exit 0
        fi
    elif [ ${MASON_SYSTEM_PACKAGE:-false} = true ]; then
        if [ -f "${MASON_PREFIX}/version" ] ; then
            mason_success "Using system-provided ${MASON_NAME} $(set -e;mason_system_version)"
            exit 0
        fi
    else
        if [ -f "${MASON_PREFIX}/${MASON_LIB_FILE}" ] ; then
            mason_success "Already installed at ${MASON_PREFIX}"
            exit 0
        fi
    fi
}


function mason_check_installed {
    # skip installing if it already exists
    if [ ${MASON_HEADER_ONLY:-false} = true ] ; then
        if [ -d "${MASON_PREFIX}" ] ; then
            return 0
        fi
    elif [ ${MASON_SYSTEM_PACKAGE:-false} = true ]; then
        if [ -f "${MASON_PREFIX}/version" ] ; then
            return 0
        fi
    elif [ -f "${MASON_PREFIX}/${MASON_LIB_FILE}" ] ; then
        return 0
    fi
    mason_error "Package ${MASON_NAME} ${MASON_VERSION} isn't installed"
    return 1
}


function mason_clear_existing {
    if [ -d "${MASON_PREFIX}" ]; then
        mason_step "Removing existing package... ${MASON_PREFIX}"
        rm -rf "${MASON_PREFIX}"
    fi
}


function mason_download {
    mkdir -p "${MASON_ROOT}/.cache"
    cd "${MASON_ROOT}/.cache"
    if [ ! -f ${MASON_SLUG} ] ; then
        mason_step "Downloading $1..."
        CURL_RESULT=0
        curl --retry 3 ${MASON_CURL_ARGS} -f -S -L "$1" -o ${MASON_SLUG}  || CURL_RESULT=$?
        if [[ ${CURL_RESULT} != 0 ]]; then
            mason_error "Failed to download ${1} (returncode: $CURL_RESULT)"
            exit $RESULT
        fi
    fi

    MASON_HASH=`git hash-object ${MASON_SLUG}`
    if [ "$2" != "${MASON_HASH}" ] ; then
        mason_error "Hash ${MASON_HASH} of file ${MASON_ROOT}/.cache/${MASON_SLUG} doesn't match $2"
        exit 1
    fi
}

function mason_setup_build_dir {
    rm -rf "${MASON_ROOT}/.build/${MASON_SLUG}"
    mkdir -p "${MASON_ROOT}/.build/"
    cd "${MASON_ROOT}/.build/"
}

function mason_extract_tar_gz {
    mason_setup_build_dir
    tar xzf ../.cache/${MASON_SLUG} $@
}

function mason_extract_tar_bz2 {
    mason_setup_build_dir
    tar xjf ../.cache/${MASON_SLUG} $@
}

function mason_extract_tar_xz {
    mason_setup_build_dir
    tar xJf ../.cache/${MASON_SLUG} $@
}

function mason_prepare_compile {
    :
}

function mason_compile {
    mason_error "COMPILE FUNCTION MISSING"
    exit 1
}

function mason_clean {
    :
}

function bash_lndir() {
    oldifs=$IFS
    IFS='
    '
    src=$(cd "$1" ; pwd)
    dst=$(cd "$2" ; pwd)
    find "$src" -type d |
    while read dir; do
            mkdir -p "$dst${dir#$src}"
    done

    find "$src" -type f -o -type l |
    while read src_f; do
            dst_f="$dst${src_f#$src}"
            if [[ ! -f $dst_f ]]; then
                ln -s "$src_f" "$dst_f"
            fi
    done
    IFS=$oldifs
}


function run_lndir() {
    # TODO: cp is fast, but inconsistent across osx
    #/bin/cp -R -n ${MASON_PREFIX}/* ${TARGET_SUBDIR}
    mason_step "Linking ${MASON_PREFIX}"
    mason_step "Links will be inside ${TARGET_SUBDIR}"
    if hash lndir 2>/dev/null; then
        mason_substep "Using $(which lndir) for symlinking"
        lndir -silent ${MASON_PREFIX}/ ${TARGET_SUBDIR} 2>/dev/null
    else
        mason_substep "Using bash fallback for symlinking (install lndir for faster symlinking)"
        bash_lndir ${MASON_PREFIX}/ ${TARGET_SUBDIR}
    fi
    mason_step "Done linking ${MASON_PREFIX}"
}

function mason_link {
    if [ ! -d "${MASON_PREFIX}" ] ; then
        mason_error "${MASON_PREFIX} not found, please install first"
        exit 0
    fi
    TARGET_SUBDIR="${MASON_ROOT}/.link/"
    mkdir -p ${TARGET_SUBDIR}
    run_lndir
}


function mason_build {
    mason_load_source

    mason_step "Building for Platform '${MASON_PLATFORM}/${MASON_PLATFORM_VERSION}'..."
    cd "${MASON_BUILD_PATH}"
    mason_prepare_compile

    if [ ${MASON_PLATFORM} = 'ios' ]; then

        SIMULATOR_TARGETS="i386 x86_64"
        DEVICE_TARGETS="armv7 arm64"
        LIB_FOLDERS=

        for ARCH in ${SIMULATOR_TARGETS} ; do
            mason_substep "Building for iOS Simulator ${ARCH}..."
            export CFLAGS="${MASON_ISIM_CFLAGS} -arch ${ARCH}"
            export CXXFLAGS="${MASON_ISIM_CFLAGS} -arch ${ARCH}"
            cd "${MASON_BUILD_PATH}"
            mason_compile
            cd "${MASON_PREFIX}"
            mv lib lib-isim-${ARCH}
            for i in lib-isim-${ARCH}/*.a ; do lipo -info $i ; done
            LIB_FOLDERS="${LIB_FOLDERS} lib-isim-${ARCH}"
        done

        for ARCH in ${DEVICE_TARGETS} ; do
            mason_substep "Building for iOS ${ARCH}..."
            export CFLAGS="${MASON_IOS_CFLAGS} -arch ${ARCH}"
            export CXXFLAGS="${MASON_IOS_CFLAGS} -arch ${ARCH}"
            cd "${MASON_BUILD_PATH}"
            mason_compile
            cd "${MASON_PREFIX}"
            mv lib lib-ios-${ARCH}
            for i in lib-ios-${ARCH}/*.a ; do lipo -info $i ; done
            LIB_FOLDERS="${LIB_FOLDERS} lib-ios-${ARCH}"
        done

        # Create universal binary
        mason_substep "Creating Universal Binary..."
        cd "${MASON_PREFIX}"
        mkdir -p lib
        for LIB in $(find ${LIB_FOLDERS} -name "*.a" | xargs basename | sort | uniq) ; do
            lipo -create $(find ${LIB_FOLDERS} -name "${LIB}") -output lib/${LIB}
            lipo -info lib/${LIB}
        done

        cd "${MASON_PREFIX}"
        rm -rf ${LIB_FOLDERS}
    elif [ ${MASON_PLATFORM} = 'android' ]; then
        cd "${MASON_BUILD_PATH}"
        mason_compile
    else
        cd "${MASON_BUILD_PATH}"
        mason_compile
    fi

    mason_success "Installed at ${MASON_PREFIX}"

    #rm -rf ${MASON_ROOT}/.build
}

function mason_config_custom {
    # Override this function in your script to add more configuration variables
    :
}

function mason_config {
    local MASON_CONFIG_CFLAGS MASON_CONFIG_LDFLAGS MASON_CONFIG_STATIC_LIBS MASON_CONFIG_PREFIX LN
    local MASON_CONFIG_INCLUDE_DIRS MASON_CONFIG_DEFINITIONS MASON_CONFIG_OPTIONS

    MASON_CONFIG_CFLAGS=$(set -e;mason_cflags)
    MASON_CONFIG_LDFLAGS=$(set -e;mason_ldflags)
    MASON_CONFIG_STATIC_LIBS=$(set -e;mason_static_libs)
    MASON_CONFIG_PREFIX="{prefix}"

    # Split up the cflags into include dirs, definitions and options.
    LN=$'\n'
    MASON_CONFIG_CFLAGS="${MASON_CONFIG_CFLAGS// -/${LN}-}"
    MASON_CONFIG_INCLUDE_DIRS=$(echo -n "${MASON_CONFIG_CFLAGS}" | sed -nE 's/^-(I|isystem) *([^ ]+)/\2/p' | uniq)
    MASON_CONFIG_DEFINITIONS=$(echo -n "${MASON_CONFIG_CFLAGS}" | sed -nE 's/^-(D) *([^ ]+)/\2/p')
    MASON_CONFIG_OPTIONS=$(echo -n "${MASON_CONFIG_CFLAGS}" | sed -nE '/^-(D|I|isystem) *([^ ]+)/!p')

    echo "name=${MASON_NAME}"
    echo "version=${MASON_VERSION}"
    if ${MASON_HEADER_ONLY}; then
        echo "header_only=${MASON_HEADER_ONLY}"
    else
        echo "platform=${MASON_PLATFORM}"
        echo "platform_version=${MASON_PLATFORM_VERSION}"
    fi
    for name in include_dirs definitions options ldflags static_libs ; do
        eval value=\$MASON_CONFIG_$(echo ${name} | tr '[:lower:]' '[:upper:]')
        if [ ! -z "${value}" ]; then
            echo ${name}=${value//${MASON_PREFIX}/${MASON_CONFIG_PREFIX}}
        fi
    done
    mason_config_custom
}

function mason_write_config {
    local INI_FILE
    INI_FILE="${MASON_PREFIX}/mason.ini"
    echo "`mason_config`" > "${INI_FILE}"
    mason_substep "Wrote configuration file ${INI_FILE}:"
    cat ${INI_FILE}
}

function mason_try_binary {
    MASON_BINARIES_DIR=`dirname "${MASON_BINARIES}"`
    mkdir -p "${MASON_ROOT}/.binaries/${MASON_BINARIES_DIR}"

    # try downloading from S3
    if [ ! -f "${MASON_BINARIES_PATH}" ] ; then
        mason_step "Downloading binary package ${MASON_BINARIES}..."
        curl --retry 3 ${MASON_CURL_ARGS} -f -L \
            https://${MASON_BUCKET}.s3.amazonaws.com/${MASON_BINARIES} \
            -o "${MASON_BINARIES_PATH}.tmp" && \
            mv "${MASON_BINARIES_PATH}.tmp" "${MASON_BINARIES_PATH}" || \
            mason_step "Binary not available yet for https://${MASON_BUCKET}.s3.amazonaws.com/${MASON_BINARIES}"
    else
        mason_step "Updating binary package ${MASON_BINARIES}..."
        curl --retry 3 ${MASON_CURL_ARGS} -f -L -z "${MASON_BINARIES_PATH}" \
            https://${MASON_BUCKET}.s3.amazonaws.com/${MASON_BINARIES} \
            -o "${MASON_BINARIES_PATH}.tmp"
        if [ $? -eq 0 ] ; then
            if [ -f "${MASON_BINARIES_PATH}.tmp" ]; then
                mv "${MASON_BINARIES_PATH}.tmp" "${MASON_BINARIES_PATH}"
            else
                mason_step "Binary package is still up to date"
            fi
        else
            mason_step "Binary not available yet for ${MASON_BINARIES}"
        fi
    fi

    # unzip the file if it exists
    if [ -f "${MASON_BINARIES_PATH}" ] ; then
        mkdir -p "${MASON_PREFIX}"
        cd "${MASON_PREFIX}"

        # Try to force the ownership of the unpacked files
        # to the current user using fakeroot if available
        `which fakeroot` tar xzf "${MASON_BINARIES_PATH}"

        if [ ! -z ${MASON_PKGCONFIG_FILE:-} ] ; then
            if [ -f "${MASON_PREFIX}/${MASON_PKGCONFIG_FILE}" ] ; then
            # Change the prefix
                MASON_ESCAPED_PREFIX=$(echo "${MASON_PREFIX}" | sed -e 's/[\/&]/\\&/g')
                sed -i.bak "s/prefix=.*/prefix=${MASON_ESCAPED_PREFIX}/" \
                    "${MASON_PREFIX}/${MASON_PKGCONFIG_FILE}"
            fi
        fi

        mason_success "Installed binary package at ${MASON_PREFIX}"
        exit 0
    fi
}


function mason_pkgconfig {
    MASON_PKGCONFIG_FILES=""
    for pkgconfig_file in ${MASON_PKGCONFIG_FILE}; do
        MASON_PKGCONFIG_FILES="${MASON_PKGCONFIG_FILES} ${MASON_PREFIX}/${pkgconfig_file}"
    done
    echo pkg-config ${MASON_PKGCONFIG_FILES}
}

function mason_cflags {
    local FLAGS
    FLAGS=$(set -e;`mason_pkgconfig` --static --cflags)
    # Replace double-prefix in case we use a sysroot.
    echo ${FLAGS//${MASON_SYSROOT}${MASON_PREFIX}/${MASON_PREFIX}}
}

function mason_ldflags {
    local FLAGS
    FLAGS=$(set -e;`mason_pkgconfig` --static --libs)
    # Replace double-prefix in case we use a sysroot.
    echo ${FLAGS//${MASON_SYSROOT}${MASON_PREFIX}/${MASON_PREFIX}}
}

function mason_static_libs {
    if [ -z "${MASON_LIB_FILE}" ]; then
        mason_substep "Linking ${MASON_NAME} ${MASON_VERSION} dynamically"
    elif [ -f "${MASON_PREFIX}/${MASON_LIB_FILE}" ]; then
        echo "${MASON_PREFIX}/${MASON_LIB_FILE}"
    else
        mason_error "No static library file '${MASON_PREFIX}/${MASON_LIB_FILE}'"
        exit 1
    fi
}

function mason_prefix {
    echo ${MASON_PREFIX}
}

function mason_version {
    if [ ${MASON_SYSTEM_PACKAGE:-false} = true ]; then
        mason_system_version
    else
        echo ${MASON_VERSION}
    fi
}

function mason_list_existing_package {
    local PREFIX RESULT
    PREFIX=$1
    RESULT=$(aws s3api head-object --bucket mason-binaries --key $PREFIX/$MASON_NAME/$MASON_VERSION.tar.gz 2>/dev/null)
    if [ ! -z "${RESULT}" ]; then
        printf "%-30s %6.1fM    %s\n" \
            "${PREFIX}" \
            "$(bc -l <<< "$(echo ${RESULT} | jq -r .ContentLength) / 1000000")" \
            "$(echo ${RESULT} | jq -r .LastModified)"
    else
        printf "%-30s %s\n" "${PREFIX}" "<missing>"
    fi
}

function mason_list_existing {
    if [ ${MASON_SYSTEM_PACKAGE:-false} = true ]; then
        mason_error "System packages don't have published packages."
        exit 1
    elif [ ${MASON_HEADER_ONLY:-false} = true ]; then
        mason_list_existing_package headers
    else
        for PREFIX in $(jq -r .CommonPrefixes[].Prefix[0:-1] <<< "$(aws s3api list-objects --bucket=mason-binaries --delimiter=/)") ; do
            if [ ${PREFIX} != "headers" -a ${PREFIX} != "prebuilt" ] ; then
                mason_list_existing_package ${PREFIX}
            fi
        done
    fi
}

function mason_publish {
    local CONTENT_TYPE DATE MD5 SIGNATURE
    if [ ! ${MASON_HEADER_ONLY:-false} = true ] && [ ! -z ${MASON_LIB_FILE:-} ] && [ ! -f "${MASON_PREFIX}/${MASON_LIB_FILE}" ]; then
        mason_error "Required library file ${MASON_PREFIX}/${MASON_LIB_FILE} doesn't exist."
        exit 1
    fi

    if [ -z "${AWS_ACCESS_KEY_ID}" ]; then
        mason_error "AWS_ACCESS_KEY_ID is not set."
        exit 1
    fi

    if [ -z "${AWS_SECRET_ACCESS_KEY}" ]; then
        mason_error "AWS_SECRET_ACCESS_KEY is not set."
        exit 1
    fi

    mkdir -p `dirname ${MASON_BINARIES_PATH}`
    cd "${MASON_PREFIX}"
    rm -rf "${MASON_BINARIES_PATH}"
    tar czf "${MASON_BINARIES_PATH}" .
    (cd "${MASON_ROOT}/.binaries" && ls -lh "${MASON_BINARIES}")
    mason_step "Uploading binary package..."

    CONTENT_TYPE="application/octet-stream"
    DATE="$(LC_ALL=C date -u +"%a, %d %b %Y %X %z")"
    MD5="$(openssl md5 -binary < "${MASON_BINARIES_PATH}" | base64)"
    SIGNATURE="$(printf "PUT\n$MD5\n$CONTENT_TYPE\n$DATE\nx-amz-acl:public-read\n/${MASON_BUCKET}/${MASON_BINARIES}" | openssl sha1 -binary -hmac "$AWS_SECRET_ACCESS_KEY" | base64)"

    curl -S -T "${MASON_BINARIES_PATH}" https://${MASON_BUCKET}.s3.amazonaws.com/${MASON_BINARIES} \
        -H "Date: $DATE" \
        -H "Authorization: AWS $AWS_ACCESS_KEY_ID:$SIGNATURE" \
        -H "Content-Type: $CONTENT_TYPE" \
        -H "Content-MD5: $MD5" \
        -H "x-amz-acl: public-read"

    echo https://${MASON_BUCKET}.s3.amazonaws.com/${MASON_BINARIES}
    curl -f -I https://${MASON_BUCKET}.s3.amazonaws.com/${MASON_BINARIES}
}

function mason_run {
    if [ "$1" == "install" ]; then
        if [ ${MASON_SYSTEM_PACKAGE:-false} = true ]; then
            mason_check_existing
            mason_clear_existing
            mason_build
            mason_write_config
            mason_success "Installed system-provided ${MASON_NAME} $(set -e;mason_system_version)"
        else
            mason_check_existing
            mason_clear_existing
            mason_try_binary
            mason_build
            mason_write_config
        fi
    elif [ "$1" == "link" ]; then
        mason_link
    elif [ "$1" == "remove" ]; then
        mason_clear_existing
    elif [ "$1" == "publish" ]; then
        mason_publish
    elif [ "$1" == "build" ]; then
        mason_clear_existing
        mason_build
        mason_write_config
    elif [ "$1" == "cflags" ]; then
        mason_check_installed
        mason_cflags
    elif [ "$1" == "ldflags" ]; then
        mason_check_installed
        mason_ldflags
    elif [ "$1" == "config" ]; then
        mason_check_installed
        mason_config
    elif [ "$1" == "static_libs" ]; then
        mason_check_installed
        mason_static_libs
    elif [ "$1" == "version" ]; then
        mason_check_installed
        mason_version
    elif [ "$1" == "prefix" ]; then
        mason_prefix
    elif [ "$1" == "existing" ]; then
        mason_list_existing
    elif [ $1 ]; then
        mason_error "Unknown command '$1'"
        exit 1
    else
        mason_error "Usage: $0 <command> <lib> <version>"
        exit 1
    fi
}
