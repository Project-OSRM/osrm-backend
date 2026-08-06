#!/usr/bin/env python3
"""Generate the libosrm C++ API reference from the public headers.

    python tools/api-docs/build.py --output build/api-docs/site

Requires `doxygen` and `node` on PATH. The renderer is fetched with
`npx --yes sourcey@3.6.5`, pinned to an exact version so the output is reproducible.

The run is hermetic with respect to the working tree: the only thing it writes inside the
repository is the `--work` directory (default `build/api-docs`, which is already ignored by
the build/ rule in .gitignore). It never edits a header.

Steps:
  1. copy the Doxyfile's INPUT paths into work/src, promoting `//` prose to `///`
     (see promote_comments.py -- line-preserving, so source links stay exact),
  2. run Doxygen there to produce XML,
  3. render the XML with Sourcey into --output.
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SOURCEY = "sourcey@3.6.5"
SOURCE_EXTS = {".h", ".hpp", ".hxx", ".ipp"}

sys.path.insert(0, HERE)
from promote_comments import promote  # noqa: E402


def run(cmd, cwd=None):
    print("$", " ".join(cmd), flush=True)
    proc = subprocess.run(cmd, cwd=cwd, shell=False)
    if proc.returncode != 0:
        sys.exit(f"failed ({proc.returncode}): {' '.join(cmd)}")


def doxyfile_inputs(path):
    """Read the INPUT list out of the Doxyfile so it is declared in exactly one place."""
    text = open(path, encoding="utf-8").read()
    text = re.sub(r"\\\s*\n", " ", text)
    for line in text.splitlines():
        if line.strip().startswith("INPUT ") or line.strip().startswith("INPUT="):
            return line.split("=", 1)[1].split()
    sys.exit(f"no INPUT line in {path}")


def stage_sources(inputs, dest):
    copied = 0
    for rel in inputs:
        src = os.path.join(REPO, rel)
        if os.path.isfile(src):
            entries = [(os.path.dirname(rel), os.path.basename(rel))]
        elif os.path.isdir(src):
            entries = []
            for base, _dirs, files in os.walk(src):
                r = os.path.relpath(base, REPO)
                entries += [(r, f) for f in files]
        else:
            sys.exit(f"Doxyfile INPUT path does not exist: {rel}")
        for rel_dir, name in entries:
            if os.path.splitext(name)[1] not in SOURCE_EXTS:
                continue
            text = open(os.path.join(REPO, rel_dir, name), encoding="utf-8",
                        errors="replace").read()
            out = promote(text)
            assert out.count("\n") == text.count("\n"), name
            os.makedirs(os.path.join(dest, rel_dir), exist_ok=True)
            with open(os.path.join(dest, rel_dir, name), "w", encoding="utf-8",
                      errors="replace", newline="") as fh:
                fh.write(out)
            copied += 1
    return copied


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--output", default=os.path.join(REPO, "build", "api-docs", "site"))
    ap.add_argument("--work", default=os.path.join(REPO, "build", "api-docs"))
    ap.add_argument("--commit", default=os.environ.get("GITHUB_SHA"),
                    help="commit to link source lines to; defaults to $GITHUB_SHA or HEAD")
    args = ap.parse_args()

    commit = args.commit or subprocess.run(
        ["git", "-C", REPO, "rev-parse", "HEAD"], capture_output=True, text=True,
        check=True).stdout.strip()
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        sys.exit(f"not a full commit sha: {commit!r}")

    work = os.path.abspath(args.work)
    src = os.path.join(work, "src")
    shutil.rmtree(src, ignore_errors=True)
    shutil.rmtree(os.path.join(work, "xml"), ignore_errors=True)
    os.makedirs(src, exist_ok=True)

    doxyfile = os.path.join(HERE, "Doxyfile")
    count = stage_sources(doxyfile_inputs(doxyfile), src)
    print(f"staged {count} headers from {commit}")
    shutil.copy(doxyfile, os.path.join(src, "Doxyfile"))
    run(["doxygen", "Doxyfile"], cwd=src)

    template = open(os.path.join(HERE, "sourcey.config.template.ts"), encoding="utf-8").read()
    with open(os.path.join(work, "sourcey.config.ts"), "w", encoding="utf-8",
              newline="\n") as fh:
        fh.write(template.replace("__COMMIT__", commit))

    out = os.path.abspath(args.output)
    shutil.rmtree(out, ignore_errors=True)
    npx = "npx.cmd" if os.name == "nt" else "npx"
    run([npx, "--yes", SOURCEY, "build", "-o", out], cwd=work)
    print(f"\nlibosrm API reference written to {out}")


if __name__ == "__main__":
    main()
