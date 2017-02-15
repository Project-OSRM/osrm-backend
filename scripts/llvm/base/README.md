## llvm

This is a package of llvm. Multiple packages are based on repackaging the binaries from this one.

### Depends

#### On Linux

 - >= Ubuntu precise
 - binutils 2.27 from mason for -flto support

#### On OS X

 - OS X >= 10.11
 - On XCode >= 8 installed to `/Applications/Xcode.app` such that `xcode-select -p` returns `/Applications/Xcode.app/Contents/Developer`

If you don't have Xcode installed in `/Applications` you can still use the clang++ in this package but you need to set:

1) `-isysroot` to point at your Mac SDK (the value returned from `xcrun --show-sdk-path`)

2) Add an include flag to point at your C++ headers which are generally at `ls $(dirname $(xcrun --find clang++))/../include/c++/v1`. For the command line tools this directory is at `/Library/Developer/CommandLineTools/usr/include/c++/v1/`

### Details of builds

 - OS X build done on OS X 10.12.1
 - Linux build done on Ubuntu precise

Builds (and rebuilds) are done using `utils/rebuild-llvm-tools.sh`

#### OS X details

On MacOS we use the apple system clang++ to compile and link against the apple system libc++.

#### Linux details

On Linux we use Ubuntu Precise in order to ensure that binaries can be run on Ubuntu Precise and newer platforms.

We link clang++ and other llvm c++ tools to libc++ (the one within the build) to avoid clang++ itself depending on a specific libstdc++ version.

The clang++ binary still defaults to building and linking linux programs against libstdc++.