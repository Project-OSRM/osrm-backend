# Issue

What issue is this PR targeting? If there is no issue that addresses the problem, please open a corresponding issue and link it here.

Please read our [documentation](https://github.com/Project-OSRM/osrm-backend/blob/master/docs/releasing.md) on release and version management.
If your PR is still work in progress please attach the relevant label.

## Tasklist

 - [ ] CHANGELOG.md entry ([How to write a changelog entry](http://keepachangelog.com/en/1.0.0/#how))
 - [ ] update relevant [Wiki pages](https://github.com/Project-OSRM/osrm-backend/wiki)
 - [ ] add tests (see [testing documentation](https://github.com/Project-OSRM/osrm-backend/blob/master/docs/testing.md)
 - [ ] review
 - [ ] adjust for comments
 - [ ] cherry pick to release branch

## Code Review Checklist - author check these when done, reviewer verify
 - [ ] Travis builds pass
 - [ ] Code formatted with `scripts/format.sh`
 - [ ] Changes have test coverage
 - [ ] New exceptions, logging, errors - are messages distinct enough to track down in the code if they get thrown in production on non-debug builds?
 - [ ] The PR is one logically integrated piece of work.  If there are unrelated changes, are they at least separate commits?
 - [ ] Commit messages - are they clear enough to understand the intent of the change if we have to look at them later?
 - [ ] Code comments - are there comments explaining the intent?
 - [ ] Relevant docs updated
 - [ ] Changelog entry if required (new features, bugfixes, behaviour changes)
 - [ ] Impact on the API surface
   - [ ] If HTTP/libosrm.o is backward compatible features, bump the minor version
   - [ ] File format changes require at minor release
   - [ ] If old clients can't use the API after changes, bump the major version

If something doesn't apply, please ~~cross it out~~

## Requirements / Relations

 Link any requirements here. Other pull requests this PR is based on?
