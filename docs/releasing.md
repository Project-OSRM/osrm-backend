# Releasing a new OSRM version

We are using http://semver.org/ for versioning with major, minor and patch versions.

## Guarantees

We are giving the following guarantees between versions:

### Major version change

- There are no guarantees about compatiblity of APIs or datasets
- Breaking changes will be noted as `BREAKING` in the changelog

### Minor version change

We may introduce forward-compatible changes: query parameters and response properties may be added in responses, but existing properties may not be changed or removed. One exception to this is the addition of new turn types, which we see as forward-compatible changes.

- Forward-compatible HTTP API
- Forward-compatible C++ library API
- Forward-compatible node-osrm API
- No compatiblity between OSRM datasets (needs new processing)

### Patch version change

- No change of query parameters or response formats
- Compatible HTTP API
- Compatible C++ library API
- Compatible node-osrm API
- Compatible OSRM datasets

## Release and branch management

- The `master` branch is for the bleeding edge development
- We create and maintain release branches `x.y` to control the release flow
- We create the release branch once we create release branches once we want to release the first RC
- RCs go in the release branch, commits needs to be cherry-picked from master
- No minor or major version will be released without a code-equal release candidates
- For quality assurance, release candidates need to be staged beforing tagging a final release
- Patch versions may be released without a release candidate
- We may backport fixes to older versions and release them as patch versions

## Releasing a version

1. Check out the appropriate release branch `x.y`
2. Make sure `CHANGELOG.md` is up to date.
3. Make sure the `package.json` on branch `x.y` has been committed.
4. Make sure all tests are passing (e.g. Github Actions CI gives you a :heavy_check_mark:)
5. Use an annotated tag to mark the release: `git tag vx.y.z -a` Body of the tag description should be the changelog entries. Commit should be one in which the `package.json` version matches the version you want to release.
6. Use `npm run docs` to generate the API documentation.  Copy `build/docs/*` to `https://github.com/Project-OSRM/project-osrm.github.com` in the `docs/vN.N.N/api` directory
7. Push tags and commits: `git push; git push --tags`
8. On https://github.com/Project-OSRM/osrm-backend/releases press `Draft a new release`,
   write the release tag `vx.y.z` in the `Tag version` field, write the changelog entries in the `Describe this release` field
   and press `Publish release`. Note that Github Actions CI deployments will create a release when publishing node binaries, so the release
   may already exist. In which case the description should be updated with the changelog entries.
9. If not a release-candidate: Write a mailing-list post to osrm-talk@openstreetmap.org to announce the release
10. Wait until the Github Actions build has been completed and check if the node binaries were published by doing:
    `rm -rf node_modules && npm install` locally.
11. For final releases run `npm publish` or `npm publish --tag next` for release candidates.
12. Bump version in `package.json` to `{MAJOR}.{MINOR+1}.0-unreleased` on the `master` branch after the release.
