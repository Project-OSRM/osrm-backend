vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ThePhD/sol2
    REF "v${VERSION}"
    SHA512 4404b124a4f331d77459c01a92cd73895301e7d3ef829a0285980f0138b9cc66782de3713d54f017d5aad7d8a11d23eeffbc5f3b39ccb4d4306a955711d385dd
    HEAD_REF develop
    PATCHES
        header-only.patch
        fix-noexcept-lua-cfunction.patch
        fix-optional-reference-emplace.patch
        lua55-compat.patch
)

# Lua 5.5 compatibility (port of #7404) for the compat headers.
# These files ship with CRLF line endings, so an in-tree .patch would need
# git attributes gymnastics to survive normalization. A string-replace is
# simpler and line-ending agnostic.
foreach(_sol2_file_and_replace
    "include/sol/compatibility/compat-5.3.h|LUA_VERSION_NUM > 504|LUA_VERSION_NUM > 505"
    "include/sol/compatibility/compat-5.4.h|LUA_VERSION_NUM == 504|LUA_VERSION_NUM >= 504")
    string(REPLACE "|" ";" _parts "${_sol2_file_and_replace}")
    list(GET _parts 0 _sol2_file)
    list(GET _parts 1 _sol2_from)
    list(GET _parts 2 _sol2_to)
    file(READ "${SOURCE_PATH}/${_sol2_file}" _sol2_contents)
    string(REPLACE "${_sol2_from}" "${_sol2_to}" _sol2_contents "${_sol2_contents}")
    file(WRITE "${SOURCE_PATH}/${_sol2_file}" "${_sol2_contents}")
endforeach()

set(VCPKG_BUILD_TYPE release) # header-only
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/sol2)
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
