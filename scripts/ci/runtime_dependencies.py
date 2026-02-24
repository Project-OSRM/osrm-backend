"""
Get the shared libraries dependencies of a target.

Incredible but true: Cmake does not offer this very elementary function: to get a list
of the dynamic libraries (.so) a target needs to run.

Example: python scripts/ci/runtime_dependencies.py --grep "boost|tbb|osrm" build/Release/nodejs/node_osrm.node
"""

import argparse
import collections
import glob
import os
import platform
import re
import subprocess
import sys

args = argparse.Namespace()

tool = None


class Scanner:
    # [dependencies, rpaths]
    def scan(self, filename) -> tuple[list, list]:
        deps = []
        rpaths = []
        stdout = subprocess.check_output(
            f"{self.tool} {filename}", shell=True, encoding="utf-8"
        )
        for line in stdout.splitlines():
            m = self.re_deps.search(line)
            if m:
                deps.append(m.group(1))
            if self.re_rpath:
                m = self.re_rpath.search(line)
                if m:
                    rpaths = m.group(1).split(":")

        return deps, rpaths


class LinuxScanner(Scanner):
    def __init__(self):
        self.tool = "objdump -p"
        # NEEDED               libosrm.so
        # NEEDED               libosrm_utils.so
        # NEEDED               libboost_iostreams.so.1.88.0
        self.re_deps = re.compile(r"NEEDED\s+(.*)$")
        # RUNPATH              $ORIGIN:/path/to/lib:/path/to/another/lib:
        self.re_rpath = re.compile(r"R[UN]*PATH\s+(.*)$")


class MachoScanner(Scanner):
    def __init__(self):
        self.tool = "otool -L"
        # @rpath/node_osrm.node (compatibility version 0.0.0, current version 0.0.0)
        # @rpath/libosrm.dylib (compatibility version 0.0.0, current version 0.0.0)
        # @rpath/libosrm_utils.dylib (compatibility version 0.0.0, current version 0.0.0)
        # @rpath/libboost_iostreams.dylib (compatibility version 0.0.0, current version 0.0.0)
        self.re_deps = re.compile(r"@rpath/(.*dylib)\s")


class WindowsScanner(Scanner):
    def __init__(self):
        # https://learn.microsoft.com/en-us/cpp/build/reference/dependents?view=msvc-170
        # https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-Readme.md
        # Image has the following dependencies:
        #   KERNEL32.dll
        #   SHLWAPI.dll
        #   boost_iostreams.dll
        #   MSVCP140.dll
        tool = glob.glob(
            r"C:\\Program Files\\Microsoft Visual Studio\\**\\DUMPBIN.EXE",
            recursive=True,
        )
        if len(tool) > 0:
            self.tool = f'"{tool[0]}" /DEPENDENTS'
        else:
            print("Cannot find dumpbin.exe")
            exit(-1)
        self.re_deps = re.compile(r"^\s+(.*dll)$")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        "filenames",
        nargs="+",
        type=str,
        metavar="FILENAME",
    )
    parser.add_argument(
        "--grep",
        type=str,
        help="regular expression the libraries must match",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="regular expression the libraries must match",
        default=False,
    )

    parser.parse_args(namespace=args)

    def debug(*a):
        if args.debug:
            print(*a)

    if platform.system() == "Linux":
        scanner = LinuxScanner()
    if platform.system() == "Darwin":
        scanner = MachoScanner()
    if platform.system() == "Windows":
        scanner = WindowsScanner()

    if args.grep:
        args.grep = re.compile(args.grep)

    def find_lib(lib: str, rpaths: list[str]) -> str:
        """Find the library in rpaths[]"""
        if "/" in lib:
            return lib
        for p in rpaths + ["/usr/lib", "/usr/lib64", "/usr/lib/x86_64-linux-gnu/"]:
            p = os.path.join(p, lib)
            if os.access(p, os.R_OK):
                return p
        return lib

    # [filename, parent RPATHs]
    queue = collections.deque()
    resolved = dict()

    for filename in args.filenames:
        queue.append((filename, []))

    while queue:
        filename, parent_rpaths = queue.popleft()
        debug("FILENAME:", filename)
        debug("  PRPATH:", parent_rpaths)
        deps, rpaths = scanner.scan(filename)
        debug("  RPATHS:", rpaths)
        rpaths = parent_rpaths + rpaths
        for dep in deps:
            if dep not in resolved:
                debug("  DEP: ", dep)
                dirname = os.path.dirname(dep)
                # be careful *not* to resolve symlinks! eg. libtbbmalloc.so.2 => libtbbmalloc.so.2.17
                path = os.path.normpath(find_lib(dep, [dirname] + rpaths))
                resolved[dep] = path
                debug("  RESOLVED: ", path)
                queue.append((path, rpaths))

    # always use \n even on Windows!
    sys.stdout.reconfigure(newline="\n")

    for lib in sorted(resolved.values()):
        if args.grep:
            if args.grep.search(lib):
                print(lib)
        else:
            print(lib)


if __name__ == "__main__":
    main()
