# Python Bindings Development Guide

## Installing for production

Pre-built wheels are published to PyPI for Linux (x86\_64, aarch64), macOS (arm64), and Windows (amd64), requiring Python 3.12+:

```bash
pip install osrm-bindings
```

To build from source (e.g. unsupported platform):

```bash
pip install osrm-bindings --no-binary osrm-bindings
```

Source builds compile the full OSRM C++ library — this takes a long time.
See [platform-specific notes](#platform-specific-build-requirements) for prerequisites.

## Installing for development

Clone the repo and install in editable mode with dev dependencies:

```bash
git clone https://github.com/Project-OSRM/osrm-backend
cd osrm-backend
pip install -e ".[dev]"
```

Install pre-commit hooks:

```bash
pre-commit install
```

## Platform-specific build requirements

### Linux

No extra steps needed. Wheels are built inside a custom manylinux image
that has all OSRM dependencies baked in. A regular source install will pull
OSRM's dependencies via the system package manager or build them from source.

### macOS

Install OSRM's C++ dependencies via Homebrew:

```bash
brew install lua tbb boost@1.90
brew link boost@1.90
```

### Windows

Windows uses [Conan](https://conan.io/) for OSRM's C++ dependencies. Install
it and generate a default profile before building:

```bash
pip install conan==2.27.0
conan profile detect --force
```

Pass `ENABLE_CONAN=ON` to CMake at build time (see below).

## Building locally

### Editable install (recommended for development)

A standard `pip install -e .` works, but by default pip uses PEP 517 isolated
builds — each invocation creates a temporary directory, compiles everything,
then discards it. This means OSRM is recompiled from scratch every time.

Use `--no-build-isolation` to make scikit-build-core reuse the persistent
build directory (`build/{wheel_tag}/`) across runs:

```bash
# Linux / macOS
pip install -e . --no-build-isolation

# Windows
pip install -e . --no-build-isolation -C cmake.define.ENABLE_CONAN=ON
```

The first run is slow (full OSRM compile). Subsequent runs only recompile
changed binding files.

::: warning Keep config flags identical across runs
scikit-build-core hashes its configuration to detect changes. If the flags
differ between runs, it wipes the build directory and starts from scratch.
:::

::: warning Generator mismatch
CMake records the generator in `CMakeCache.txt`. If you ever see
`Does not match the generator used previously`, delete the build directory
and rebuild from scratch:
```powershell
Remove-Item -Recurse -Force build/cp312-abi3-win_amd64
```
:::

### Building a wheel

After the editable install has compiled everything, produce a wheel without
recompiling:

```bash
# Linux / macOS
pip wheel . --no-build-isolation -w dist

# Windows
pip wheel . --no-build-isolation -C cmake.define.ENABLE_CONAN=ON -w dist
```

CMake finds the existing artifacts in the build directory and skips
recompilation. The wheel lands in `dist/`.

### Wheel repair

Locally built wheels link against system shared libraries and are tagged
as `linux_x86_64` (not `manylinux`). To make them portable or to inspect
their dependencies, use the platform-specific repair tools:

**Linux** — [auditwheel](https://github.com/pypa/auditwheel):

```bash
pip install auditwheel
auditwheel show dist/*.whl          # inspect shared library dependencies
auditwheel repair -w dist dist/*.whl # bundle libs and retag as manylinux
```

**macOS** — [delocate](https://github.com/matthew-brett/delocate):

```bash
pip install delocate
delocate-listdeps dist/*.whl        # inspect dependencies
delocate-wheel -w dist dist/*.whl   # bundle dylibs
```

**Windows** — [delvewheel](https://github.com/adang1345/delvewheel):

```bash
pip install delvewheel
delvewheel show dist/*.whl          # inspect dependencies
delvewheel repair -w dist dist/*.whl
```

On Windows, Conan's shared DLLs (tbb, hwloc) must be on PATH for delvewheel
to find them. Activate the `conanrun.bat` generated in the build tree first.

::: tip
cibuildwheel runs wheel repair automatically in CI. You only need these
commands when building wheels locally for distribution.
:::

### Compiler cache

On Linux and macOS, ccache is used automatically (pre-installed in the
manylinux image; installed via Homebrew for macOS CI).

On Windows, scikit-build-core defaults to the **Visual Studio generator**,
which does not support `CMAKE_CXX_COMPILER_LAUNCHER`. The build dir reuse
from `--no-build-isolation` is the main speed optimisation for local Windows
development.

## Running tests

Build the test data (requires the package to be installed so the `osrm`
executables are available):

```bash
# Linux / macOS
cd test/data && make

# Windows
cd test\data && windows-build-test-data.bat
```

Load the shared memory datastore:

```bash
python -m osrm datastore test/data/ch/monaco
```

Run the test suite:

```bash
pytest test/python/
```

## Running cibuildwheel locally

[cibuildwheel](https://cibuildwheel.pypa.io/) builds wheels inside isolated
environments that closely match CI. Install it with:

```bash
pip install cibuildwheel
```

Build for the current platform:

```bash
cibuildwheel --platform linux    # requires Docker on non-Linux hosts
cibuildwheel --platform macos
cibuildwheel --platform windows
```

Wheels land in `wheelhouse/`.

**Windows note:** cibuildwheel's `config-settings` in pyproject.toml are
*replaced* (not merged) by `CIBW_CONFIG_SETTINGS_WINDOWS` if that env var is
set. Always include `ENABLE_CONAN=ON` explicitly when overriding via the env
var.

**Linux note:** The manylinux container mounts the host ccache directory via a
Docker volume. Set `CCACHE_DIR` on the host so the mount path matches what CI
uses:

```bash
CIBW_CONTAINER_ENGINE="docker; create_args: --volume /tmp/ccache:/ccache" \
CIBW_ENVIRONMENT_LINUX="CCACHE_DIR=/ccache" \
cibuildwheel --platform linux
```

## Type stubs

`src/python/osrm/osrm_ext.pyi` is auto-generated by `nanobind_add_stub()` at
build time and committed to the repository so documentation tools can work
without compiling the extension.

After changing C++ bindings, rebuild and commit the updated stub:

```bash
pip install -e . --no-build-isolation   # regenerates the .pyi
git add src/python/osrm/osrm_ext.pyi
```

To regenerate manually without a full rebuild:

```bash
pip install nanobind ruff
python -m nanobind.stubgen -m osrm.osrm_ext -o src/python/osrm/osrm_ext.pyi
ruff format src/python/osrm/osrm_ext.pyi
```

## Releasing

Releases are driven by git tags. `setuptools-scm` reads the tag to set the
package version — no manual version bumps needed.

1. Ensure CI is green on `main`.
2. Create and push an annotated tag:
   ```bash
   git tag -a v1.2.3 -m "v1.2.3"
   git push origin v1.2.3
   ```
3. The publish workflow triggers on tag push, builds wheels for all platforms,
   and uploads to PyPI via trusted publisher.
4. Verify the release at [pypi.org/project/osrm-bindings](https://pypi.org/project/osrm-bindings/).

**Test release without tagging:** trigger the publish workflow manually via
`workflow_dispatch` with the `upload` input set to `true`.
