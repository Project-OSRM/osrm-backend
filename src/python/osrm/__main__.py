import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

IS_WIN = platform.system().lower() == "windows"

# Executables are installed under osrm/bin/ inside the package directory.
# For editable/dev installs, fall back to the CMake build directory or PATH.
_BIN_DIR = Path(__file__).parent / "bin"

# delvewheel bundles shared DLLs (tbb12, hwloc) into osrm_bindings.libs/.
# Subprocess-launched executables can't benefit from the .pyd DLL path
# patching, so we pass PATH explicitly on Windows.
_LIBS_DIR = Path(__file__).parent.parent / "osrm_bindings.libs"

COMMANDS = {
    "components": "osrm-components",
    "contract": "osrm-contract",
    "customize": "osrm-customize",
    "datastore": "osrm-datastore",
    "extract": "osrm-extract",
    "partition": "osrm-partition",
    "routed": "osrm-routed",
}

if len(sys.argv) < 2 or sys.argv[1] not in COMMANDS:
    print("Usage: python -m osrm <command> [args...]")
    print(f"Commands: {', '.join(COMMANDS)}")
    sys.exit(1)

exe_name = COMMANDS[sys.argv[1]]
if IS_WIN:
    exe_name += ".exe"


def _find_executable(exe_name):
    # 1. Installed wheel layout: osrm/bin/
    candidate = _BIN_DIR / exe_name
    if candidate.is_file():
        return candidate

    # 2. Editable install: look in CMake build directories
    project_root = Path(__file__).parent.parent.parent.parent
    for build_dir in sorted(project_root.glob("build/*/"), reverse=True):
        candidate = build_dir / exe_name
        if candidate.is_file():
            return candidate

    # 3. System PATH
    found = shutil.which(exe_name)
    if found:
        return Path(found)

    return None


executable = _find_executable(exe_name)
if not executable:
    print(f"OSRM executable not found: {exe_name}")
    sys.exit(1)

cmd = [str(executable)] + sys.argv[2:]

env = None
if IS_WIN and _LIBS_DIR.is_dir():
    env = {**os.environ, "PATH": str(_LIBS_DIR) + os.pathsep + os.environ.get("PATH", "")}

proc = subprocess.run(cmd, env=env)
sys.exit(proc.returncode)
