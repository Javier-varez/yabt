{
  description = "yabt";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
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
        in
        rec {
          yabt = pkgs.stdenv.mkDerivation {
            pname = "yabt";
            version = "0.0.1";

            src = ./.;

            buildInputs = with pkgs; [
              gcc
              (luajit.withPackages (ps: with ps; [ busted ]))
              pkg-config
              gnumake
            ];
            nativeBuildInputs = with pkgs; [
              ninja
              git
            ];
            buildPhase = ''
              make -j$(nproc)
            '';
            installPhase = ''
              install -m 0755 build/yabt $out
            '';
          };
          default = yabt;
        }
      );
      devShells = forEachSystem (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };
        in
        rec {
          yabt = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [
              # build deps (busted is needed for testing)
              gcc
              (luajit.withPackages (ps: with ps; [ busted ]))
              pkg-config

              # Runtime deps
              ninja
              git

              # dev tools
              bear
              gdb
            ];
          };

          default = yabt;
        }
      );
    };
}
