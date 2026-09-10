# A commit on main rather than v0.1.0, which predates both the scratch allocator
# fix the extractor needs to classify a country's worth of ways without growing
# guest memory, and the segment rejection that made the line and box queries
# affordable. Move to the next tag once one is cut.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Project-OSRM/gauche-rs
    REF b1c3af3029975ddbe573bad874c4520db77321d0
    SHA512 b1ad5bbb45ea714005ffe8c2f34bde694448e507d8307818113b31a7288e6958b747660d614337004ea793b82a5ba1fbe02449dee7d06e271a4527741d147840
    HEAD_REF main
    PATCHES
        # w2c2 emits its atomic helpers whether or not the module uses any, and
        # gauche's uses none. Its MSVC branch hands the _Interlocked intrinsics a
        # U8* where they want volatile char*, short*, long* or __int64*, which
        # the GCC branch casts for and MSVC rejects. Drop this once the fix is
        # upstream in w2c2 and regenerated here.
        fix-msvc-interlocked-casts.patch
)

# The C++ wrapper is the consumable surface; cpp/CMakeLists.txt reaches back up
# to c/ for the w2c2-transpiled wasm it compiles alongside.
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/cpp"
    OPTIONS
        -DGAUCHE_BUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME gauche CONFIG_PATH lib/cmake/gauche)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
