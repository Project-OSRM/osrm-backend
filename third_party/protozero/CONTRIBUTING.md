# Contributing to protozero

## Releasing

To release a new protozero version:

 - Make sure all tests are passing locally, on travis and on appveyor
 - Make sure "make doc" builds
 - Update version number in
   - include/protozero/version.hpp (two places)
   - CMakeLists.txt (one place)
 - Update CHANGELOG.md
   (don't forget links at the bottom of the file)
 - Update UPGRADING.md if necessary
 - `git commit -m "Release X.Y.Z" include/protozero/version.hpp CMakeLists.txt CHANGELOG.md UPGRADING.md`
 - `git tag vX.Y.Z`
 - `git push`
 - `git push --tags`
 - Go to https://github.com/mapbox/protozero/releases
   and edit the new release. Put "Version x.y.z" in title and
   cut-and-paste entry from CHANGELOG.md.
