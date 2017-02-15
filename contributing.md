# Release process

- Increment version at the top of `mason`
- Update changelog
- Ensure tests are passing
- Tag a release:

```
TAG_NAME=$(cat mason | grep MASON_RELEASED_VERSION= | cut -c25-29)
git tag v${TAG_NAME} -a -m "v${TAG_NAME}" && git push --tags
```

- Go to https://github.com/mapbox/mason/releases/new and create a new release
