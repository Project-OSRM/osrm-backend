# Contributing to vtzero

## Releasing

To release a new vtzero version:

 - Make sure all tests are passing locally and on travis/appveyor
 - Update version number in
   - `CMakeLists.txt` (one place)
   - `include/vtzero/version.hpp` (two places)
 - Update CHANGELOG.md
 - Update UPGRADING.md if necessary
 - `git commit -m "Release X.Y.Z" include/vtzero/version.hpp CMakeLists.txt CHANGELOG.md UPGRADING.md`
 - `git tag vX.Y.Z`
 - `git push`
 - `git push --tags`
 - Go to https://github.com/mapbox/vtzero/releases
   and edit the new release. Put "Version x.y.z" in title and
   cut-and-paste entry from CHANGELOG.md.

## Updating submodules

Call `git submodule update --recursive --remote` to update to the newest
version of the mvt fixtures used for testing.

