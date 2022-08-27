{
  inputs = {
    nixpkgs.url = "github:NixOs/nixpkgs/nixos-unstable";

  };
  outputs = inputs @ {
    self, 
    nixpkgs, 
    ... 
  }: let
    inherit (nixpkgs) lib;
    genSystems = lib.genAttrs [
      # Supported platform
      "x86_64-linux"
    ];
    pkgsFor = nixpkgs.legacyPackages;
    system = "x86_64-linux";
    pkgs = pkgsFor.${system};

  in {
    # Overlays
    overlays.default = final: prev: {
      principia = prev.callPackage ./nix/default.nix {};
    };

    packages = genSystems (system:
      (self.overlays.default null pkgsFor.${system}) 
      // { default = self.packages.${system}.principia; }
    );

    devShells = genSystems (system: {
      default = pkgsFor.${system}.mkShell {
        name = "principia-build-env";
        inputsFrom = [
          self.packages.${system}.principia
        ];
      };
    });
  };
}
