"""Promote a libosrm header's plain `//` prose into Doxygen `///` doc comments.

The public headers document themselves with ordinary `//` comments sitting directly above
the declaration they describe. Doxygen ignores those, so a plain Doxygen run over the public
headers yields signatures with no prose. This filter promotes a run of `//` lines to `///`
**only when the run is immediately followed by a declaration**, so the prose that is already
in the tree lands on the symbol it already describes. Nothing is invented, and nothing is
edited in the repository: the promotion happens on a throwaway copy under the build
directory.

The transform is line-preserving -- every promoted line keeps its original line number -- so
the source links Doxygen emits still point at the right line of the real header.

Deliberately conservative. A run is left alone when it:
  * is the file banner (the first comment block in the file, i.e. the BSD licence header),
  * carries a tooling directive (clang-format / NOLINT / cppcheck / SPDX / @file),
  * looks like commented-out code,
  * is not immediately followed by a declaration.

Usage:  python tools/api-docs/promote_comments.py <file>   # writes the result to stdout
"""

from __future__ import annotations

import io, re, sys

CODEISH = re.compile(
    r"^\s*(?:#|}|\)|template\s*<|typedef\b|using\b|return\b|if\b|for\b|while\b|else\b)"
    r"|[;{}]\s*$")
DIRECTIVE = re.compile(r"clang-format|NOLINT|cppcheck|SPDX|coverity|codespell|@file", re.I)
DECL = re.compile(
    r"^\s*(?:QPDF_DLL\b|QPDF_DLL_CLASS\b|template\s*<|class\b|struct\b|enum\b|union\b|namespace\b"
    r"|typedef\b|using\b|static\b|virtual\b|explicit\b|constexpr\b|inline\b|friend\b"
    r"|[A-Za-z_~][A-Za-z0-9_:<>,\s\*&\[\]]*\s*\()")
COMMENT = re.compile(r"^(\s*)//(?!/)(?!!)(.*)$")


def promote(text: str) -> str:
    lines = text.split("\n")
    out = list(lines)
    n = len(lines)
    i = 0
    first_block = True
    while i < n:
        m = COMMENT.match(lines[i])
        if not m:
            if lines[i].strip():
                first_block = False
            i += 1
            continue
        start = i
        while i < n and COMMENT.match(lines[i]):
            i += 1
        end = i  # exclusive
        block = lines[start:end]
        nxt = lines[end] if end < n else ""

        if first_block:                       # file banner / licence
            first_block = False
            continue
        body = " ".join(COMMENT.match(b).group(2) for b in block)
        if not body.strip():
            continue
        if DIRECTIVE.search(body):
            continue
        if any(CODEISH.search(COMMENT.match(b).group(2)) for b in block):
            continue                          # commented-out code
        if not nxt.strip() or not DECL.match(nxt):
            continue                          # not attached to a declaration
        for k in range(start, end):
            g = COMMENT.match(lines[k])
            out[k] = f"{g.group(1)}///{g.group(2)}"
    return "\n".join(out)


def main() -> None:
    path = sys.argv[1]
    with io.open(path, encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace", newline="\n")
    sys.stdout.write(promote(text))


if __name__ == "__main__":
    main()
