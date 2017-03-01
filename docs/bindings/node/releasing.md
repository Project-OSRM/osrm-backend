# Releasing

Releasing a new version of `node-osrm` is mostly automated using Travis CI.

The version of `node-osrm` is locked to the same version as `osrm-backend`. Every `node-osrm` should have a `osrm-backend` release of the same version. Of course, only release a `node-osrm` after the release has been tagged in `osrm-backend`.

These steps all happen on `master`. After the release is out, create a branch using the MAJOR.MINOR version of the release to document code changes made for that version.

### Steps to release

1. Update the `osrm_release` field in `package.json` to the corresonding git tag in `osrm-backend.`
   
   Confirm the desired OSRM branch and commit to `master`.

1. Bump node-osrm version

   Update the `CHANGELOG.md` and the `package.json` version if needed.

1. Check that Travis CI [builds are passing](https://travis-ci.org/Project-OSRM/node-osrm) for the latest commit on `master`.

1. Publishing binaries

   If travis builds are passing then it's time to publish binaries by committing with a message containing `[publish binary]`. Use an empty commit for this.

   ```
   git commit --allow-empty -m "[publish binary] vMAJOR.MINOR.PATCH"
   ```

1. Test

   Locally you can now test binaries. Cleanup, re-install, and run the tests like:

   ```
   make clean
   npm install # will pull remote binaries
   npm ls # confirm deps are correct
   make test
   ```

1. Tag

   Once binaries are published for Linux and OS X then its time to tag a new release and add the changelog to the tag:

   ```
   git tag vMAJOR.MINOR.PATCH -a
   git push --tags
   ```

1. Publish node-osrm. **we only do this for stable releases**

   First ensure your local `node-pre-gyp` is up to date:

   ```
   npm ls
   ```

   This is important because it is bundled during packaging.

   If you see any errors then do:

   ```
   rm -rf node_modules/node-pre-gyp
   npm install node-pre-gyp
   ```

   Now we're ready to publish `node-osrm` to <https://www.npmjs.org/package/osrm>:

   ```
   npm publish
   ```

   Dependent apps can now pull from the npm registry like:

   ```
   "dependencies": {
       "osrm": "^MAJOR.MINOR.PATCH"
   }
   ```

   Or can still pull from the github tag like:

   ```
   "dependencies": {
       "osrm": "https://github.com/Project-OSRM/node-osrm/archive/vMAJOR.MINOR.PATCH.tar.gz"
   }
   ```
