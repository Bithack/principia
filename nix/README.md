# Building using Nix/NixOS

Only tested on x64 systems.

## With flakes

In your system configuration's flake directory, add this repository as input.

```nix
# flake.nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    principia.url = "github:Bithack/principia";
    # Use the currently installed nixpkgs to build principia
    principia.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, principia }: {
    nixosConfiguration = {
      HOSTNAME = nixpkgs.lib.nixosSystem {
        system = "x86_64-linux";
        modules = [
          {
            nixpkgs.overlays = [ principia.overlays.default ];
            environment.systemPackages = with pkgs; [
              principia # Use overlay 
            ];
          }
        ];
      };
    };
  };
}

```

## Without flakes

In your `configuration.nix`, add the package to install globally.

```nix
# configuration.nix
{ config, pkgs, ... }: let
  flake-compat = builtins.fetchTarball "https://github.com/edolstra/flake-compat/archive/master.tar.gz"; 
  principia = (import flake-compat {
    src = builtins.fetchTarball "https://github.com/Bithack/principia/archive/master.tar.gz";
  }).defaultNix;
in {
  nixpkgs.overlays = [ principia.overlays.default ];
  environment.systemPackages = with pkgs; [
    principia
  ];
}
```

## With home-manager

if you're using home-manager, you can install Principia into your user profile 
by adding the package into `home.nix`.

```nix
# home.nix
{ config, pkgs, inputs, ... }: { 
  nixpkgs.overlays = [ inputs.principia.overlays.default ];
  home.packages = with pkgs; [
    principia
  ];
}

```

Rebuild your system/home configuration.
