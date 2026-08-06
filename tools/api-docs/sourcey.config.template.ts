// Rendering config for the generated libosrm C++ API reference.
//
// tools/api-docs/build.py copies this next to the Doxygen XML and substitutes __COMMIT__
// with the commit being documented, so every symbol links to the exact file and line it was
// read from.
//
// Exported as a plain object rather than through sourcey's `defineConfig` helper: the
// renderer is invoked with `npx sourcey@3.6.5`, so there is no local node_modules for an
// `import { defineConfig } from "sourcey"` to resolve against. `defineConfig` is an identity
// helper that exists for editor types, and the schema below is what sourcey reads either way.
export default ({
  name: "libosrm C++ API",
  repo: "https://github.com/Project-OSRM/osrm-backend",
  editBranch: "__COMMIT__",
  navigation: {
    tabs: [
      {
        tab: "API Reference",
        slug: "api",
        doxygen: {
          xml: "xml",
          language: "cpp",
          groups: false,
          index: "flat",
          sourceUrl:
            "https://github.com/Project-OSRM/osrm-backend/blob/__COMMIT__/",
        },
      },
    ],
  },
  navbar: {
    links: [
      { type: "github", href: "https://github.com/Project-OSRM/osrm-backend" },
    ],
  },
  footer: {
    links: [
      { type: "github", href: "https://github.com/Project-OSRM/osrm-backend" },
    ],
  },
});
