# A commit on main rather than v0.1.0, which predates the scratch allocator fix
# the extractor needs to classify a whole country's worth of ways without
# growing guest memory without bound. Move to the next tag once one is cut.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Project-OSRM/gauche-rs
    REF 093cee8fa8cfcd51248dfe4a6fb2aa06335da32c
    SHA512 ce2613238aa5c558ea9aa3c5e35dd4acfdcea58112498dd86bbb76c5d2225814d15e3843bdb467669f2ec3bfbadd9c295652a271dc49b55b0a55ffdbfc0629a8
    HEAD_REF main
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
