# Releasing a new OSRM version

We use a **YYYY-MM-patchlevel** versioning scheme for all releases, with a dual-format approach:
- **Git tags & releases:** YYYY-MM-patchlevel format (e.g., `v2026-04-0`, `v2026-04-1`)
- **npm package:** Semver-compatible format (YYYY-2000).MM.patchlevel (e.g., `26.4.0`, `26.4.1`)

This approach allows git tags to use human-readable year-month formatting while maintaining npm/node-pre-gyp compatibility.

## Versioning Scheme

### Format

**Git Tags:** `vYYYY-MM-patchlevel` (e.g., v2026-04-0)
- **YYYY:** Current year (4 digits)
- **MM:** Current month (01-12)
- **patchlevel:** Incremental counter starting at 0 per month (0, 1, 2, ...)

**npm Package:** `(YYYY-2000).MM.patchlevel` (e.g., 26.4.0)
- **YYYY-2000:** Year offset (fits in uint8_t for fingerprinting)
- **MM:** Month (01-12)
- **patchlevel:** Counter (0, 1, 2, ...)

### Examples

Git tags and npm versions:
- April 2026, 1st release: tag `v2026-04-0`, npm `26.4.0`
- April 2026, 2nd release: tag `v2026-04-1`, npm `26.4.1`
- May 2026, 1st release: tag `v2026-05-0`, npm `26.5.0`

## Release Compatibility Guarantees

### Patch version change (new patchlevel in same month)

- No change of query parameters or response formats
- Compatible HTTP API
- Compatible C++ library API
- Compatible node-osrm API
- Compatible OSRM datasets

### Month change (new YYYY-MM)

- May introduce forward-compatible changes: query parameters and response properties may be added in responses, but existing properties may not be changed or removed
- Forward-compatible HTTP API
- Forward-compatible C++ library API
- Forward-compatible node-osrm API
- No compatibility between OSRM datasets (needs new processing)

## Release Management

- The `master` branch is for development and should always be green
- **Automated monthly releases** occur on the **1st of each month at 08:00 UTC**
- All changes in master will be automatically released monthly
- No release candidates are used; the master branch is the quality gate
- Patch versions within the same month can be released manually at any time

## Automated Release Process

Releases are created automatically every month on a scheduled basis:

1. A GitHub Actions workflow runs on the 1st of each month at 08:00 UTC
2. Version is calculated as `YYYY-MM-patchlevel`
3. `package.json` version is updated
4. A git tag is created and pushed (e.g., `v2026-04-0`)
5. A GitHub Release is published with auto-generated release notes
6. The package is published to npm

## Manual Release Trigger

You can also trigger a release manually on any branch:

1. Go to **Actions** → **Monthly Release** workflow
2. Click **Run workflow**
3. Select your branch (defaults to `master`)
4. Optionally override the version (format: `YYYY-MM-patchlevel`)
5. Click **Run workflow**

This is useful for:
- Out-of-schedule patch releases within the same month
- Emergency releases from other branches
- Backports to older versions

## Release Checklist

When releasing (automated or manual):

1. ✅ All GitHub Actions CI checks pass
2. ✅ The target branch is in a releasable state
3. ✅ For manual releases: verify the version format is correct (`YYYY-MM-patchlevel`)
4. ✅ The release is created automatically with:
   - Git tag
   - GitHub Release (with auto-generated release notes)
   - npm publication

## After Release

No additional manual steps are required. The automated workflow handles:
- Version bumping in `package.json`
- Git commit and tag creation
- GitHub Release publishing
- npm package publication

For non-automated releases, monitor:
- GitHub Actions to verify the release completed successfully
- npm registry to confirm the new version is published

