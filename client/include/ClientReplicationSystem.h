#pragma once
#include "ClientSimulation.h"
#include "SnapshotHistory.h"
#include <deque>

class ClientWorld;
class ClientReplicationSystem {
public:
  ClientReplicationSystem(ClientWorld &world, ClientSimulation &simulation);
  bool HandlePacket(Buffer &buff, double transitSeconds = 0);
  // a reliable message from the server: always a game event
  bool HandleMessage(Buffer &buff);
  void Update(double dt);
  PlayerID localID = 0;
  std::deque<GameEvent> events; // newest at the back, game pops them

private:
  ClientWorld &world;
  ClientSimulation &simulation;
  SnapshotHistory history;
  bool ApplyWorldSnapshot(const WorldSnapshot &snapshot, double transitSeconds);
};
