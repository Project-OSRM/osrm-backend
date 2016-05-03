# Releasing a new OSRM version

Do decide if this is a major or minor version bump use: http://semver.org/

What we guarantee on major version changes:

- Breaking changes will be in the changelog
- If we break an HTTP API we bump the version

What we guarantee on minor version changes:

- HTTP API does not include breaking changes
- C++ library API does not include breaking changes
- node-osrm API does not include breaking changes

What we DO NOT guarantee on minor version changes:

- file format comp ability. Breakage will be listed in the changelog.
- new turn types and fields may be introduced. How to handle this see [the HTTP API docs](http.md).

What we guarantee on patch version changes:

- HTTP API does not include breaking changes
- C++ library API does not include breaking changes
- node-osrm API does not include breaking changes
- full file format compatibility

## Major or Minor release x.y

1. Make sure all tests are passing (e.g. Travis CI gives you a :thumbs_up:)
2. Make sure `CHANGELOG.md` is up to date.
3. Make sure the OSRM version in `CMakeLists.txt` is up to date
4. Use an annotated tag to mark the release: `git tag vx.y.0 -a` Body of the tag description should be the changelog entries.
5. Push tags and commits: `git push; git push --tags`
6. Branch of the `vx.y.0` tag to create a release branch `x.y`:
   `git branch x.y. vx.y.0; git push -u x.y:origin/x.y`
7. Modify `.travis.yml` to allow builds for the `x.y` branch.
8. Write a mailing-list post to osrm-talk@openstreetmap.org to announce the release

## Patch release x.y.z

1. Check out the appropriate release branch x.y
2. Make sure all fixes are listed in the changelog and included in the branch
3. Make sure all tests are passing (e.g. Travis CI gives you a :thumbs_up:)
4. Make sure the OSRM version in `CMakeLists.txt` is up to date
5. Use an annotated tag to mark the release: `git tag vx.y.z -a` Body of the tag description should be the changelog entries.
6. Push tags and commits: `git push; git push --tags`
7. Proceede with the `node-osrm` release as outlined in the repository.
8. Write a mailing-list post to osrm-talk@openstreetmap.org to announce the release

