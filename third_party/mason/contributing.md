# Release process

- Increment version at the top of `mason`
- Increment version in test/unit.sh
- Update changelog
- Ensure tests are passing
- Tag a release: `git tag v0.1.0 -a -m "v0.1.0" && git push --tags`
- Go to https://github.com/mapbox/mason/releases/new and create a new release
