# Python Bindings Development Guide

## Installing for production

Pre-built wheels are published to PyPI for Linux (x86\_64), macOS (x86\_64),
and Windows (amd64). They use the CPython 3.12 stable ABI (`cp312-abi3`) and
therefore install on Python 3.12+:

```bash
pip install osrm-bindings
```

The package itself supports Python 3.10+ when built from source — needed for
3.10/3.11, aarch64 Linux, arm64 macOS, or any platform without a pre-built
wheel:

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

Releases are driven by the monthly release workflow
([.github/workflows/release-monthly.yml](../../.github/workflows/release-monthly.yml)),
not by pushing a tag by hand. The workflow bumps the version, creates the tag,
drives CI, downloads the built wheels, and publishes to both PyPI and npm in
one shot.

### Scheduled monthly release

A cron on the 1st of each month at 08:00 UTC runs the workflow against
`master`:

1. Compute the next version as `(YYYY-2000).M.patchlevel` (e.g. `26.4.0`).
2. Bump `package.json` + `package-lock.json`, commit, create annotated tag
   `v<version>`, push branch and tag.
3. Dispatch `osrm-backend.yml` on the tag. That run builds wheels + sdist
   via `cibuildwheel` and uploads them as `wheels-*` artifacts.
4. Wait for the dispatched CI run to finish with conclusion `success`.
5. Run the `publish` job: download every `wheels-*` artifact into `dist/`,
   publish to PyPI via trusted publisher (OIDC), then `npm publish`.

If PyPI fails, the npm publish still runs (the npm steps have
`if: ${{ !cancelled() }}`), and the overall job is marked failed so the PyPI
problem stays visible.

### Manual release

Trigger the workflow from the Actions UI or `gh workflow run release-monthly.yml`
with optional inputs:

- `version_override` — set the version explicitly (e.g. `26.4.1`) instead of
  using the `(YYYY-2000).M.patchlevel` calculation.
- `branch` — release from a branch other than `master`.

### Verification

After the run finishes, check:

- Tag `v<version>` exists and the GitHub Release is published.
- [pypi.org/project/osrm-bindings](https://pypi.org/project/osrm-bindings/) shows
  the new version with an sdist and three platform wheels (manylinux x86_64,
  macOS x86_64, win_amd64).
- [npmjs.com/package/@project-osrm/osrm](https://www.npmjs.com/package/@project-osrm/osrm)
  shows the matching version.

### Version mechanics

`pyproject.toml` uses `setuptools-scm` with `local_scheme = "no-local-version"`.
On a tag checkout (e.g. `v26.4.0`), the Python version resolves cleanly to
`26.4.0`, matching the `package.json` version that `release-monthly.yml`
committed when creating the tag.
