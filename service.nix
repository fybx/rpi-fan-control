{ config, pkgs, ... }:

let
  rpi-fan-control-pkg = pkgs.callPackage ./default.nix { };
in
{
  systemd.services.rpi-fan-control = {
    description = "Raspberry Pi Fan Control Service";
    after = [ "network.target" ];
    wantedBy = [ "multi-user.target" ];

    serviceConfig = {
      ExecStart = "${rpi-fan-control-pkg}/bin/rpi-fan-control";
      User = "root";
      Group = "root";

      Restart = "on-failure";
      RestartSec = "5s";
    };
  };
}