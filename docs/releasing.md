# Releasing a new OSRM version

## Major or Minor release x.y

Do decide if this is a major or minor version bump use: http://semver.org/

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

