# Releasing a new OSRM version

We use a **unified semver versioning scheme** for monthly releases: `(YYYY-2000).M.patchlevel`
- **Format:** `X.M.patchlevel` where X = year - 2000, M = month (1-12, no leading zeros)
- **Example:** `26.4.0` represents April 2026, first release
- **Git tags:** Prefixed with `v` (e.g., `v26.4.0`)
- **npm packages:** Unprefixed semver (e.g., `26.4.0`)

## Version History

**Previous scheme (ended 2025):** Traditional semantic versioning (v6.0, v6.0.1, v6.0.2, etc.)
- Last release: v6.0.0 in December 2025
- Manual release process

**New scheme (started 2026):** Monthly date-based versioning with automated releases
- First release: v26.1.0 (January 2026)
- Automatic monthly releases on the 1st of each month at 08:00 UTC
- Year offset: 2026 → 26, 2027 → 27, etc.
- Month: 1-12 (no leading zeros), patch: 0-N per month

## Versioning Scheme

### Format

**Git tags:** `vX.M.patchlevel` where X = year - 2000, M = 1-12
- **X:** Year offset (26 = 2026, 27 = 2027, etc.)
- **M:** Month (1-12, no leading zeros)
- **patchlevel:** Incremental counter starting at 0 per month (0, 1, 2, ...)

**npm packages:** `X.M.patchlevel` (same as git tag without the `v` prefix)

### Examples

Git tags and npm versions for the same release:
- April 2026, 1st release: Git tag `v26.4.0`, npm `26.4.0`
- April 2026, 2nd release: Git tag `v26.4.1`, npm `26.4.1`
- May 2026, 1st release: Git tag `v26.5.0`, npm `26.5.0`
- January 2027, 1st release: Git tag `v27.1.0`, npm `27.1.0`

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

## Conventional Commits

Pull request titles must follow [Conventional Commits](https://www.conventionalcommits.org/) format with types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `ci`, `chore`, `build`. This is validated in CI and helps organize the commit history.

Breaking changes should be indicated with the `!` suffix in the PR title (e.g., `feat!: remove deprecated API`) so they are called out in release notes.

## Release Management

- The `master` branch is for development and should always be green
- **Automated monthly releases** occur on the **1st of each month at 08:00 UTC**
- All changes in master will be automatically released monthly
- No release candidates are used; the master branch is the quality gate
- Patch versions within the same month can be released manually at any time

## Automated Release Process

Releases are created automatically every month on a scheduled basis:

1. A GitHub Actions workflow runs on the 1st of each month at 08:00 UTC
2. Version is calculated as `(YYYY-2000).M.patchlevel` with M = 1-12 (no leading zeros)
3. `package.json` and `package-lock.json` versions are updated
4. A git tag is created and pushed (e.g., `v26.4.0`)
5. A GitHub Release is published with auto-generated release notes
6. The package is published to npm (format: `26.4.0` without `v` prefix)

## Manual Release Trigger

You can also trigger a release manually on any branch:

1. Go to **Actions** → **Monthly Release** workflow
2. Click **Run workflow**
3. Select your branch (defaults to `master`)
4. Optionally override the version (format: `X.M.patchlevel` with M = 1-12, e.g., `26.4.0`)
5. Click **Run workflow**

This is useful for:
- Out-of-schedule patch releases within the same month
- Emergency releases from other branches
- Backports to older versions

## Release Checklist

When releasing (automated or manual):

1. ✅ All GitHub Actions CI checks pass
2. ✅ The target branch is in a releasable state
3. ✅ For manual releases: verify the version format is correct (`X.M.patchlevel` with month 1-12, e.g., `26.4.0`)
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

