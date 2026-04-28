vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mapbox/vtzero
    REF v1.2.0
    SHA512 ddf3b6e61ffe18d5fd8c7e9c4a3e84a0e70854d56fd1303838b763eb8cd10abdb74079bd5c676fa9eb55d990b030ce4da9d94363fb03ca503d14da8f9e97a3b7
    HEAD_REF master
)

set(VCPKG_BUILD_TYPE release) # header-only
vcpkg_cmake_configure(SOURCE_PATH ${SOURCE_PATH})

vcpkg_cmake_install()

vcpkg_install_copyright(
    FILE_LIST "${SOURCE_PATH}/LICENSE"
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
