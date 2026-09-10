# TODO: repin to the release tag once Project-OSRM/gauche-rs#10 lands. Until then
# this is that PR's head, which carries the scratch allocator fix the extractor
# needs to classify a planet's worth of ways without growing guest memory.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Project-OSRM/gauche-rs
    REF 515f8fe6c99f66fcf965f3bb8dc583d02e1f103e
    SHA512 ce971484712bd97fa0971b437a6373b5b3907d8635db147d3e4d3df7c37facdf476a9e4ab935e86acc7a25d172c699942f217e662fd1151cf323c8618d30c5f3
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
