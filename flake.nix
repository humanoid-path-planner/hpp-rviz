{
  description = "Rviz plugins for hpp visualization";

  inputs.gepetto.url = "github:gepetto/nix";

  outputs =
    inputs:
    inputs.gepetto.lib.mkFlakoboros inputs (
      { lib, ... }:
      {
        rosOverrideAttrs.hpp-rviz = {

          src = lib.fileset.toSource {
            root = ./.;
            fileset = lib.fileset.unions [
              ./CMakeLists.txt
              ./doc
              ./package.xml
              ./pyproject.toml
              ./rviz_common_plugins.xml
              ./src
            ];
          };

        };
      }
    );
}
