set -e
PACKAGE_JSON_VERSION=$(node -e "console.log(require('./package.json').version)")
echo PUBLISH=$([[ "${GITHUB_REF:-}" == "refs/tags/v${PACKAGE_JSON_VERSION}" ]] && echo "On" || echo "Off") >> $GITHUB_ENV