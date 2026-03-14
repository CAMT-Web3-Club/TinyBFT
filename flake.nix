{
  description = "TinyBFT development environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs =
    {
      self,
      nixpkgs,
    }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      mbedtls-custom = pkgs.mbedtls.overrideAttrs (old: {
        version = "v3.6.5";
      });
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        buildInputs = [
          pkgs.cmake
          pkgs.gnumake
          pkgs.gcc
          mbedtls-custom
        ];
      };
    };
}
