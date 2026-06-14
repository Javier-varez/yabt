{
  description = "yabt";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
  };

  outputs =
    {
      nixpkgs,
      ...
    }:
    let
      systems = [
        "aarch64-darwin"
        "aarch64-linux"
        "x86_64-linux"
      ];

      forEachSystem = fn: nixpkgs.lib.genAttrs systems (system: fn system);

    in
    {
      packages = forEachSystem (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };
          luaEnv = pkgs.luajit.withPackages (ps: with ps; [
            luarocks
            busted
          ]);
          luarocksConfig = pkgs.writeText "luarocks-config.lua" ''
            rocks_trees = {
              { name = "nix", root = "${luaEnv}" },
              { name = "luarocks", root = "${pkgs.luajitPackages.luarocks}" },
            }
          '';
          yabt-pkg = { toolchain ? "gcc" }: pkgs.stdenv.mkDerivation rec {
            pname = "yabt";
            version = "0.0.1";

            LUAROCKS_CONFIG = luarocksConfig;

            src = pkgs.runCommand "make-src" {
              nativeBuildInputs = [ pkgs.git ];
            } "mkdir -p $out/yabt && cp -r ${./.}/* $out/yabt/ && git -C $out/yabt/ init";

            sourceRoot = "${src}/yabt";

            buildInputs = with pkgs; [
              gcc
              clang
              llvm
              pkg-config
              gnumake
              cacert
              curl
            ];
            nativeBuildInputs = with pkgs; [
              ninja
              git
              luaEnv
            ];

            buildPhase = ''
              make -j$(nproc)
              ./make_build/yabt sync
              ./make_build/yabt build --build-dir=BUILD_STAGE1 '//yabt/.*' cc_toolchain=${toolchain}
              ./BUILD_STAGE1/src/yabt/yabt build --build-dir=BUILD_STAGE2 '//yabt/.*' cc_toolchain=${toolchain}
            '';
            installPhase = ''
              install -d $out/bin
              install -m 0755 BUILD_STAGE2/src/yabt/yabt $out/bin/yabt
            '';
          };
        in
        rec {
          yabt = yabt-pkg {};
          yabt-clang = yabt-pkg { toolchain = "clang"; };
          default = yabt;
        }
      );
      devShells = forEachSystem (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };
          luaEnv = pkgs.luajit.withPackages (ps: with ps; [
            luarocks
            busted
          ]);
          luarocksConfig = pkgs.writeText "luarocks-config.lua" ''
            rocks_trees = {
              { name = "nix", root = "${luaEnv}" },
              { name = "luarocks", root = "${pkgs.luajitPackages.luarocks}" },
            }
          '';
        in
        rec {
          yabt = pkgs.mkShell {
            LUAROCKS_CONFIG = luarocksConfig;

            nativeBuildInputs = with pkgs; [
              # build deps (busted is needed for testing)
              clang
              llvm
              gcc
              luaEnv
              pkg-config

              # Runtime deps
              ninja
              git

              # dev tools
              bear
              gdb
              clang-tools
            ];
          };

          default = yabt;
        }
      );
    };
}
