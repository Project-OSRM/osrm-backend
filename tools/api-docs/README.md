# libosrm C++ API reference generator

`docs/libosrm.md` has, since 2016, described the libosrm contract by telling readers where to
find each type on GitHub. This directory turns those same headers into a browsable reference,
published at <https://project-osrm.org/libosrm-api/>.

```sh
python tools/api-docs/build.py                       # -> build/api-docs/site
python tools/api-docs/build.py --output /tmp/api     # anywhere else
```

Requirements: `doxygen` and `node` on `PATH`. Nothing is added to the C++ build, no dependency
is added to the project, and no header is modified — the renderer is fetched on demand with
`npx --yes sourcey@3.6.5` and everything intermediate is written under `build/`, which is
already ignored.

## What runs

| Step | File | What it does |
|---|---|---|
| 1 | `promote_comments.py` | Copies the headers into `build/api-docs/src`, turning the `//` prose that already sits above a declaration into `///` so Doxygen picks it up. Line-preserving, so the source links stay exact. Conservative: file banners, tooling directives and commented-out code are left alone. |
| 2 | `Doxyfile` | XML-only Doxygen run over the public headers. `EXTRACT_ALL = NO`, so an undocumented symbol stays out of the reference rather than showing up as a bare signature. |
| 3 | `sourcey.config.template.ts` | Rendering config. `__COMMIT__` is substituted with `$GITHUB_SHA`, `--commit`, or `git rev-parse HEAD`, so every symbol links to `blob/<commit>/<file>#L<line>`. |

## Scope

`INPUT` in the `Doxyfile` is deliberately the set of headers `docs/libosrm.md` points at, plus
the types those headers expose in their signatures:

`include/osrm/osrm.hpp`, `include/engine/api/**`, `include/engine/{status,engine_config,approach,bearing,hint}.hpp`,
`include/util/{coordinate,json_container}.hpp`, `include/storage/storage_config.hpp`.

Pointing Doxygen at `include/` as a whole would pull in the extractor, partitioner, customizer
and internal engine trees, which are not part of the library contract embedders code against.

## Publishing

The output is copied into the `libosrm-api/` directory of
[`Project-OSRM/project-osrm.github.com`](https://github.com/Project-OSRM/project-osrm.github.com),
minus `_og/` (GitHub Pages runs Jekyll on that repository and Jekyll drops leading-underscore
paths; those files are only social-preview images). That repository already stores the built
documentation for each release, so the reference lives alongside it.
