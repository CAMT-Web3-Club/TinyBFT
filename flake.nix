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
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        buildInputs = [
          # Build tools
          pkgs.cmake
          pkgs.gnumake
          pkgs.gcc
          pkgs.gtest
          pkgs.mbedtls
          pkgs.openssl
          pkgs.pkg-config

          # ESP32 tools
          pkgs.esptool
          pkgs.screen
        ];
      };
    };
}
