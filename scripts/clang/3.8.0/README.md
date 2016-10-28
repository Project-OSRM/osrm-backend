### Clang++ v3.8.0

This clang++ package depends on and defaults to compiling C++ programs against libstdc++.

For clang++ itself to be able to run and compile C++ programs you need to upgrade the libstdc++ version.

You also need to upgrade the libstdc++ for the programs to run that you compile with this version of clang++.

You can do this on Travis like:

```yml
      addons:
        apt:
          sources: [ 'ubuntu-toolchain-r-test' ]
          packages: [ 'libstdc++-5-dev' ]
```

You can do this on any debian system like:

```sh
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update -y
apt-get install -y libstdc++-5-dev
```

A full example of installing clang and upgrading libstdc++ on travis is:

```yml

language: generic

matrix:
  include:
    - os: linux
      sudo: false
      env: CXX=clang++
      addons:
        apt:
          sources: [ 'ubuntu-toolchain-r-test' ]
          packages: [ 'libstdc++-5-dev' ]

install:
  - git clone --depth 1 https://github.com/mapbox/mason
  - ./.mason/mason install clang 3.8.0
  - export PATH=$(./.mason/mason prefix clang 3.8.0)/bin:${PATH}
  - which clang++
```

Note: Installing `libstdc++-5-dev` installs a library named `libstdc++6`. This is not version 6, it is the ABI 6. Note that there is no dash between the `++` and the `6` like there is between the `++` and the `5` in the dev package. So don't worry about the mismatch of `5` and `6`. While the package name is based on the g++ version (`5`) the actual library version used, at the time of this writing, is `v6.1.1` (this comes from https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test/+packages). The `6` again is ABI not version: even the libstdc++ `v4.6.3` package (the default on Ubuntu precise) is named/aliased to `libstdc++6`

If you hit a runtime error like `/usr/lib/x86_64-linux-gnu/libstdc++.so.6: version GLIBCXX_3.4.20' not found` it means you forgot to upgrade libstdc++6 to at least `v6.1.1`.
