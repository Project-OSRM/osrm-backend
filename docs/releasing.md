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
3. Make sure the OSRM version in `CMakeLists.txt` is up to date
4. Make sure the `package.json` is up to date.
5. Make sure all tests are passing (e.g. Travis CI gives you a :thumbs_up:)
6. Use an annotated tag to mark the release: `git tag vx.y.z -a` Body of the tag description should be the changelog entries.
7. Use `npm run build-api-docs` to generate the API documentation.  Copy `build/docs/*` to `https://github.com/Project-OSRM/project-osrm.github.com` in the `docs/vN.N.N/api` directory
8. Push tags and commits: `git push; git push --tags`
9. If not a release-candidate: Write a mailing-list post to osrm-talk@openstreetmap.org to announce the release
10. Wait until the travis build has been completed and check if the node binaries were published by doing:
    `rm -rf node_modules && npm install` locally.
11. For final releases run `npm publish` or `npm publish --tag next` for release candidates.

