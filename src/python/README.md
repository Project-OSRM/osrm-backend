# osrm-bindings

**Python bindings for [osrm-backend](https://github.com/Project-OSRM/osrm-backend) using [nanobind](https://github.com/wjakob/nanobind).**

## PyPI

```
pip install osrm-bindings
```

> [!NOTE]
> On PyPI we only distribute `abi3` wheels for each platform, i.e. one needs at least Python 3.12 to install the wheels. Of course it'll fall back to attempt an installation from source.

Platform | Arch
---|---
Linux | x86_64
Linux | aarch64
MacOS | arm64
Windows | x86_64

## From Source

osrm-bindings requires **CPython 3.10+** and can be installed from source by running the following command in the repository root:

```
pip install .
```

The Python bindings are built alongside the OSRM C++ libraries. The version of the bindings matches the version of osrm-backend.

## Example

The following example will showcase the process of calculating routes between two coordinates.

First, import the `osrm` library, and instantiate an instance of OSRM:
```python
import osrm

# Instantiate osrm_py instance
osrm_py = osrm.OSRM("./test/data/ch/monaco.osrm")
```

Then, declare `RouteParameters`, and then pass it into the `osrm_py` instance:
```python
# Declare Route Parameters
route_params = osrm.RouteParameters(
    coordinates = [(7.41337, 43.72956), (7.41546, 43.73077)]
)

# Pass it into the osrm_py instance
res = osrm_py.Route(route_params)

# Print out result output
print(res["waypoints"])
print(res["routes"])
```

## Type Stubs

The file `src/python/osrm/osrm_ext.pyi` contains auto-generated type stubs for the C++ extension module. These are used by IDEs for autocompletion and by documentation tools without compiling the extension.

After changing C++ bindings, rebuild the project to regenerate the stubs:

```
pip install -e .
```

Then commit the updated `.pyi` file.
