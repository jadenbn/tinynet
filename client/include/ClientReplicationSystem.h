#pragma once
#include "ClientSimulation.h"
#include "Protocol.h"

class ClientWorld;
class ClientReplicationSystem {
public:
  ClientReplicationSystem(ClientWorld &world, ClientSimulation &simulation);
  bool HandlePacket(Buffer &buff);
  PlayerID localID = 0;

private:
  ClientWorld &world;
  ClientSimulation &simulation;
  bool hasSnapshot = false;
  uint32_t latestTick = 0;
  bool ApplyWorldSnapshot(const WorldSnapshot &snapshot);
};
