## Open Source Routing Machine

OSRM (Open Source Routing Machine) is a high performance routing engine written in C++ designed to run on OpenStreetMap data.
It provides various routing services including route finding, distance/duration tables, GPS trace matching, and traveling salesman problem solving.
It is accessible via HTTP API, C++ library interface, and Node.js wrapper.
OSRM supports two routing algorithms: Contraction Hierarchies (CH) and Multi-Level Dijkstra (MLD).

## Developing

We target C++20 but need to deal with older compilers that only have partial support.

Use `./scripts/format.sh` to format the code. Ensure clang-format-15 is available on the system.

## Coding Standards

Specific practices to follow (see [wiki](https://github.com/Project-OSRM/osrm-backend/wiki/Coding-Standards)):

- **No `using namespace`**: Always use explicit `std::` prefixes, never `using namespace std;` (especially in headers)
- **Naming conventions**:
  - Type names (classes, structs, enums): `UpperCamelCase` (e.g. `TextFileReader`)
  - Variables: `lower_case_with_underscores`, private members start with `m_` (e.g. `m_node_count`)
  - Functions: `lowerCamelCase` verb phrases (e.g. `openFile()`, `isValid()`)
  - Enumerators and public members: `UpperCamelCase` (e.g. `VK_Argument`)
- **Include order**: (1) Module header, (2) Local headers, (3) Parent directory headers, (4) System includes - sorted lexicographically within each group
- **Comments**: Minimal use of comments on internal interfaces. Use C++ style `//` comments, not C style `/* */`. Use `#if 0` / `#endif` to comment out blocks
- **Integer types**: Use only `int`/`unsigned` for 32-bit, or precise-width types like `int16_t`, `int64_t`
- **No RTTI/exceptions**: Avoid `dynamic_cast<>` and exceptions (except for IO operations)
- **Compiler warnings**: Treat all warnings as errors - fix them, don't ignore them
- **Assert liberally**: Use `BOOST_ASSERT` to check preconditions and assumptions
- **Indentation**: 4 spaces (enforced by clang-format)

## Building

This project uses CMake. The build directory should be `build` or `build-{something}` or `build/{something}/`. For example:
- `./build`
- `./build-gcc-debug`
- `./build/gcc-debug`
- `./build/gcc`
- `./build/clang-debug`
- `./build-clang-release` 

Use `cmake --build ${BUILD_DIR} --target ${TARGET} -j $(nproc --ignore=2)` to build a target.


## Running

OSRM supports two data preparations: Contraction Hierarchies (CH) and Multi-Level Dijkstra (MLD).

For CH:
1. `./${BUILD_DIR}/osrm-extract -p profiles/car.lua data.osm.pbf`
2. `./${BUILD_DIR}/osrm-partition data.osrm` (optional, improves CH performance due to cache locality of node reordering)
3. `./${BUILD_DIR}/osrm-contract data.osrm`
4. `./${BUILD_DIR}/osrm-routed -a CH data.osrm`

For MLD:
1. `./${BUILD_DIR}/osrm-extract -p profiles/car.lua data.osm.pbf`
2. `./${BUILD_DIR}/osrm-partition data.osrm`
3. `./${BUILD_DIR}/osrm-customize data.osrm`
4. `./${BUILD_DIR}/osrm-routed -a MLD data.osrm`

A dataset can be both MLD and CH if both `osrm-contract` and `osrm-customize` are executed:
1. `./${BUILD_DIR}/osrm-extract -p profiles/car.lua data.osm.pbf`
2. `./${BUILD_DIR}/osrm-partition data.osrm`
3. `./${BUILD_DIR}/osrm-contract data.osrm`
4. `./${BUILD_DIR}/osrm-customize data.osrm`
5. `./${BUILD_DIR}/osrm-routed -a CH data.osrm` OR `./${BUILD_DIR}/osrm-routed -a MLD data.osrm`


## Testing

### Running Unit tests

Unit tests are contained in `unit_tests/` using boost test and can be build with:
```bash
cmake --build ${BUILD_DIR} --target tests
```

This will create various tests in `${BUILD_DIR}/unit_tests/`.

### Running Integration Tests

Integration tests are contained in `features/` using cucumber-js

To run cucumber tests with the full test matrix across different algorithms (CH, MLD) and load methods (mmap, directly, datastore).
```bash
npm test
```

To run cucumber tests for a specific configuration:
```bash
npx cucumber-js -p home -p ch -p datastore
npx cucumber-js -p home -p mld -p mmap
```

**IMPORTANT: Profile changes MUST follow this checklist:**

1. **Cucumber tests**: Every change to a profile (`.lua` files in `profiles/`) must be accompanied by a corresponding cucumber test in `features/` that validates the new or changed behavior.
   - Run the relevant cucumber tests before committing profile changes:
     ```bash
     OSRM_BUILD_DIR=build npx cucumber-js -p home -p mld -p mmap features/<profile>/<feature>.feature
     ```
     The test cache is keyed by a content hash of all profiles, binaries, and the feature file, so profile changes automatically invalidate the cache. Old cache directories can optionally be cleaned up with `rm -rf test/cache`.
   - Run the whole `.feature` file (not just the new scenario) to verify no regressions.
   - Use the existing test patterns: `routability should be` tables where any column header starting with `forw`, `backw`, or `bothw` (including `*_rate` variants) is an expectation column (not an OSM tag). Headers matching `node/<tag>` apply to node tags. All other columns become OSM way tags.
   - Do NOT add `@todo` to new scenarios — verify the expected behavior matches actual routing output.

2. **taginfo.json**: If modifying tag handling (adding, removing, or changing how OSM tags are parsed), update `taginfo.json` with any new OSM tags that are now recognized or any tags whose semantics have changed.

3. **OSM Wiki adherence**: Verify that the behavior matches [OSM tagging conventions](https://wiki.openstreetmap.org/wiki/Key:access):
   - Mode-specific tags (e.g. `bicycle=yes`) override vehicle-level restrictions (e.g. `vehicle=no`)
   - Directional suffixes (`:forward`/`:backward`) scope a tag to a specific travel direction
   - The access tag hierarchy is: mode-specific > vehicle-level > general access
   - When in doubt, consult the relevant OSM wiki pages for the tags being changed


## Python Bindings

Python bindings live under `src/python/` using nanobind + scikit-build-core.

### Layout

- `src/python/CMakeLists.txt` — nanobind module definition, installs everything under `osrm/` namespace
- `src/python/osrm/` — Python package (`__init__.py`, `__main__.py`, `.pyi` stubs)
- `src/python/src/` — C++ nanobind source files (16 files)
- `src/python/include/python/` — C++ binding headers (`.hpp`)

### Key details

- `ENABLE_PYTHON_BINDINGS=ON` triggers `add_subdirectory(src/python)` in root CMakeLists.txt
- Links against the in-tree `osrm` target directly (no FetchContent)
- Binding headers use `#include "python/..."` prefix; OSRM headers use `engine/...` paths (not the `osrm/` forwarding headers), except `osrm/osrm.hpp`
- LTO disabled for `osrm_ext` target (nanobind `NB_MAKE_OPAQUE` + GCC LTO + `-Werror` = ODR violation)
- Wheel installs executables to `osrm/bin/`, profiles to `osrm/share/profiles/`; `wheel.exclude` filters out root CMake install artifacts (`lib/`, `include/`, `bin/`, `share/`)
- `__main__.py` finds executables: `osrm/bin/` (wheel) → `build/*/` (editable) → PATH
- Type stubs (`osrm_ext.pyi`) auto-generated by `nanobind_add_stub()`; rebuild + commit after C++ changes
- C++ formatted by project clang-format; Python by ruff (both via `.pre-commit-config.yaml`)
- When changing the Python binding API (function signatures, classes, exposed attributes, parameter semantics), update [docs/python/api.md](docs/python/api.md) in the same change
- When changing the build, packaging, or release flow (pyproject.toml, CMake options, cibuildwheel config, `.github/workflows/release-monthly.yml`, `.github/workflows/osrm-backend.yml` Python steps), update [docs/python/development.md](docs/python/development.md) in the same change

### Building & testing

```bash
pip install -e ".[dev]"                    # editable install
cd test/data && make                       # build test data
python -m osrm datastore test/data/ch/monaco
pytest test/python/
```

## Contributing

<IMPORTANT>
Use the [PR template](.github/PULL_REQUEST_TEMPLATE.md) to build the PR description.
Remove irrelevant tasks, but NEVER remove `review` or submit it checked.
When creating PRs via `gh`, do not pass escaped `\n` in `--body`; use a real multiline body (heredoc or `--body-file`) so GitHub renders actual line breaks.
Disclose AI participation in the PR description by stating only the harness name and model (e.g. "GitHub Copilot, Claude Sonnet 4.6"), add a robot emoji. Do not include links, fake email addresses, or session identifiers.

Do NOT include any AI attribution (e.g. `Co-authored-by:` trailers) in git commit messages.
</IMPORTANT>
