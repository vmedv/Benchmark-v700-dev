{
  description = "C++ Template";

  inputs = {
    nixpkgs.url = "nixpkgs";
    systems.url = "github:nix-systems/x86_64-linux";
    flake-utils = {
      url = "github:numtide/flake-utils";
      inputs.systems.follows = "systems";
    };
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      
      mkShell = compilerStdenv: pkgs.mkShell {
        nativeBuildInputs = with pkgs; [
          cmake
          pkg-config
          clang-tools
        ] ++ [
          compilerStdenv.cc
          compilerStdenv.cc.bintools
        ];

        buildInputs = with pkgs; [ numactl ];

        shellHook = let
          cc = "${compilerStdenv.cc}/bin/${compilerStdenv.cc.targetPrefix}cc";
          cxx = "${compilerStdenv.cc}/bin/${compilerStdenv.cc.targetPrefix}c++";
        in ''
          # export CC=${cc}
          # export CXX=${cxx}
          export PREF_PS1="bench"
        '';
      };

    in {
      devShells = {
        gcc = mkShell pkgs.gcc15Stdenv;
        clang = mkShell pkgs.llvmPackages_21.stdenv;
        default = self.devShells.${system}.gcc;
      };
    });
}
