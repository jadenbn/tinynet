#pragma once
#include "ClientSimulation.h"
#include "SnapshotHistory.h"

class ClientWorld;
class ClientReplicationSystem {
public:
  ClientReplicationSystem(ClientWorld &world, ClientSimulation &simulation);
  bool HandlePacket(Buffer &buff, double transitSeconds = 0);
  void Update(double dt);
  PlayerID localID = 0;

private:
  ClientWorld &world;
  ClientSimulation &simulation;
  SnapshotHistory history;
  bool ApplyWorldSnapshot(const WorldSnapshot &snapshot, double transitSeconds);
};
