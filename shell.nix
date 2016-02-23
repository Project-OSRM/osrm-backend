# Nix development environment for reproducible builds.
# Enter development environment via `nix-shell`.
#
# Resources:
# - http://nixos.org/nix/
# - https://nixos.org/nix/manual/#chap-quick-start
# - https://nixos.org/nixpkgs/manual/

with import <nixpkgs> {}; {
  devEnv = stdenv.mkDerivation {
    name = "osrm-backend";
    buildInputs = [ cmake boost tbb stxxl luabind lua expat bzip2 zlib ];
  };
}
