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
      devShells = forEachSystem (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };
        in
        rec {
          vds = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [
              gcc

              # build deps (busted is needed for testing)
              (luajit.withPackages (ps: with ps; [ busted ]))
              pkg-config

              # dev tools
              bear
              gdb
            ];
          };

          default = vds;
        }
      );
    };
}
