{
  description = "Rviz plugins for hpp visualization";

  inputs.gepetto.url = "github:gepetto/nix";

  outputs =
    inputs:
    inputs.gepetto.lib.mkFlakoboros inputs (
      { ... }:
      {
        rosPackages.hpp-rviz = ./package.nix;
      }
    );
}
