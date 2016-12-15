### llvm v3.9.0

This is a package of all llvm tools. Multiple packages are based on repackaging the binaries from this one including:

  - clang++ 3.9.0
  - clang-tidy 3.9.0
  - clang-format 3.9.0

### Details of package

This package inherits from `scripts/llvm/base/common.sh`, which is a base configuration and not an actual package itself.

Therefore to see the intricacies of how this package is built look in `scripts/llvm/base/common.sh`.

The `scripts/llvm/base/common.sh` is used by both llvm 3.8.1 and llvm 3.9.0 packages (and future ones). It reduces duplication of package code and makes it easier to update the builds in one place to benefit all versions.

The `scripts/llvm/3.9.0/script.sh` therefore sources `scripts/llvm/base/common.sh` and then overrides just the functions that need to be customized. At the time of writing this is only the `setup_release` function. It is customized in order to provide version specific md5 values which can ensure that upstream packages do not change without us noticing.

### Details of builds

 - Done on Sept 22, 2016 by @springmeyer
 - OS X build done on OS X 10.11
 - Linux build done on ubuntu precise

The order and method of building was generally:

```sh
VERSION=3.9.0
./mason build llvm ${VERSION}
./mason publish llvm ${VERSION}
./mason build clang++ ${VERSION}
./mason publish clang++ ${VERSION}
./mason build clang-tidy ${VERSION}
./mason publish clang-tidy ${VERSION}
./mason build clang-format ${VERSION}
./mason publish clang-format ${VERSION}
./mason build lldb ${VERSION}
./mason publish lldb ${VERSION}
./mason build llvm-cov ${VERSION}
./mason publish llvm-cov ${VERSION}
```

#### OSX details

On macOS we use the apple system clang++ to compile and link against the apple system libc++.

```sh
$ clang++ -v
Apple LLVM version 7.3.0 (clang-703.0.31)
Target: x86_64-apple-darwin15.6.0
Thread model: posix
InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
```

Therefore the llvm package was built like:

```sh
./mason build llvm 3.9.0
```

#### Linux details

On linux we use precise in order to ensure that binaries can be run on ubuntu precise and newer platforms.

We still want to compile clang++ in c++11 mode so we compile clang++ using a mason provided clang++.

To support c++11 we'd normally need to upgrade libstdc++ which would make it problematic for the resulting binary to be portable to linux systems without that upgraded libstdc++. So we avoid this issue by instead linking clang++ to libc++, which itself will travel inside the clang++ package.

To accomplish this (c++11 build of clang++ and linking to libc++ instead of an upgraded libstdc++) we did:

```sh
./mason install clang++ 3.9.0
CLANG_39_PREFIX=$(./mason prefix clang++ 3.9.0)
export CXX=${CLANG_39_PREFIX}/bin/clang++-3.9
export CC=${CLANG_39_PREFIX}/bin/clang-3.9
./mason build llvm 3.9.0
```

The reason `LD_LIBRARY_PATH` is set before the build is so that libc++.so can be found during linking. Once linked clang++ automatically uses the right @rpath on macOS and linux to find its own libc++.so at runtime.

TODO:

 - Modify the llvm 3.9.0 build scripts to build libc++ and libc++abi first.
 - Then we should be able to point LD_LIBRARY_PATH at llvm 3.9.0's libc++.so to avoid potential ABI mismatch with the version provided by 3.8.0
 - LD_LIBRARY_PATH would be pointed at the theoretical location of llvm 3.9.0's libc++, and that location would exist and become valid before llvm 3.9.0's clang++ is linked.


### Testing of these new tools

Once these packages were built I needed to test that they could be used to build and publish other mason packages. And I needed to be able to test that these packages worked for downstream users.

To accomplish this I branched mason and changed the `MASON_PLATFORM_ID` to enable publishing new binaries with llvm 3.9.0 tools without overwriting existing binaries.

```
diff --git a/mason.sh b/mason.sh
index 800216d..ccc1fcb 100644
--- a/mason.sh
+++ b/mason.sh
@@ -259,7 +259,7 @@ MASON_SLUG=${MASON_NAME}-${MASON_VERSION}
 if [[ ${MASON_HEADER_ONLY} == true ]]; then
     MASON_PLATFORM_ID=headers
 else
-    MASON_PLATFORM_ID=${MASON_PLATFORM}-${MASON_PLATFORM_VERSION}
+    MASON_PLATFORM_ID=${MASON_PLATFORM}-${MASON_PLATFORM_VERSION}-llvm390
 fi
 MASON_PREFIX=${MASON_ROOT}/${MASON_PLATFORM_ID}/${MASON_NAME}/${MASON_VERSION}
 MASON_BINARIES=${MASON_PLATFORM_ID}/${MASON_NAME}/${MASON_VERSION}.tar.gz
```

But changing the `MASON_PLATFORM_ID` also means that you loose access to all existing binaries for building packages. To ensure I could still bootstrap the builds I seeded this new `MASON_PLATFORM_ID` with key build tools, including the previously built llvm tools being discussed here.

```
CUSTOM_PATH="llvm390"
aws s3 sync --acl public-read s3://mason-binaries/linux-x86_64/llvm/ s3://mason-binaries/linux-x86_64-{CUSTOM_PATH}/llvm/
aws s3 sync --acl public-read s3://mason-binaries/linux-x86_64/clang++/ s3://mason-binaries/linux-x86_64-{CUSTOM_PATH}/clang++/
aws s3 sync --acl public-read s3://mason-binaries/linux-x86_64/cmake/ s3://mason-binaries/linux-x86_64-{CUSTOM_PATH}/cmake/
aws s3 sync --acl public-read s3://mason-binaries/linux-x86_64/ccache/ s3://mason-binaries/linux-x86_64-{CUSTOM_PATH}/ccache/
aws s3 sync --acl public-read s3://mason-binaries/linux-x86_64/ninja/ s3://mason-binaries/linux-x86_64-{CUSTOM_PATH}/ninja/
aws s3 sync --acl public-read s3://mason-binaries/osx-x86_64/llvm/ s3://mason-binaries/osx-x86_64-{CUSTOM_PATH}/llvm/
aws s3 sync --acl public-read s3://mason-binaries/osx-x86_64/clang++/ s3://mason-binaries/osx-x86_64-{CUSTOM_PATH}/clang++/
aws s3 sync --acl public-read s3://mason-binaries/osx-x86_64/cmake/ s3://mason-binaries/osx-x86_64-{CUSTOM_PATH}/cmake/
aws s3 sync --acl public-read s3://mason-binaries/osx-x86_64/ccache/ s3://mason-binaries/osx-x86_64-{CUSTOM_PATH}/ccache/
aws s3 sync --acl public-read s3://mason-binaries/osx-x86_64/ninja/ s3://mason-binaries/osx-x86_64-{CUSTOM_PATH}/ninja/
```
